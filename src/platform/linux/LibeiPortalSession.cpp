#include "src/platform/linux/IEisInputSession.h"

#include <libei.h>
#include <liboeffis.h>

#include <linux/input-event-codes.h>
#include <poll.h>
#include <unistd.h>

#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

using Clock = std::chrono::steady_clock;

struct DeviceRecord {
    ei_device* device{nullptr};
    bool resumed{false};
    bool emulating{false};
    std::vector<EisRegion> regions;
};

class LibeiPortalSession final : public IEisInputSession {
public:
    ~LibeiPortalSession() override { shutdown(); }

    bool initialize() override;
    bool refresh() override;
    EisCapabilities capabilities() const override;
    std::vector<EisRegion> regions() const override;
    bool moveAbsolute(const DesktopPoint& point) override;
    bool button(bool pressed) override;
    bool scrollVertical(int logicalNotches) override;
    void shutdown() override;
    std::string lastError() const override { return m_lastError; }

private:
    bool dispatchPortal();
    bool dispatchEi();
    bool setupEi();
    bool pump(int timeoutMs);
    bool ready() const;
    bool fail(const std::string& error);
    DeviceRecord* find(ei_device* device);
    DeviceRecord* active(enum ei_device_capability capability);
    void remove(ei_device* device);
    void teardown();

    oeffis* m_oeffis{nullptr};
    ei* m_ei{nullptr};
    std::vector<DeviceRecord> m_devices;
    std::uint32_t m_sequence{0};
    bool m_portalConnected{false};
    bool m_eiConnected{false};
    bool m_initialized{false};
    std::string m_lastError;
};

bool LibeiPortalSession::fail(const std::string& error) {
    m_lastError = error;
    return false;
}

DeviceRecord* LibeiPortalSession::find(ei_device* device) {
    for (auto& record : m_devices)
        if (record.device == device) return &record;
    return nullptr;
}

DeviceRecord* LibeiPortalSession::active(
    enum ei_device_capability capability) {
    for (auto& record : m_devices) {
        if (record.resumed &&
            ei_device_has_capability(record.device, capability)) {
            return &record;
        }
    }
    return nullptr;
}

void LibeiPortalSession::remove(ei_device* device) {
    for (auto it = m_devices.begin(); it != m_devices.end(); ++it) {
        if (it->device != device) continue;
        ei_device_unref(it->device);
        m_devices.erase(it);
        return;
    }
}

bool LibeiPortalSession::setupEi() {
    const int fd = oeffis_get_eis_fd(m_oeffis);
    if (fd < 0)
        return fail(std::string("ConnectToEIS did not return a usable fd: ") +
                    std::strerror(errno));
    m_ei = ei_new_sender(nullptr);
    if (!m_ei) {
        ::close(fd);
        return fail("ei_new_sender failed");
    }
    ei_configure_name(m_ei, "c0ntrol");
    const int rc = ei_setup_backend_fd(m_ei, fd);
    if (rc != 0) {
        return fail(std::string("ei_setup_backend_fd failed: ") +
                    std::strerror(-rc));
    }
    m_portalConnected = true;
    return true;
}

bool LibeiPortalSession::dispatchPortal() {
    oeffis_dispatch(m_oeffis);
    while (true) {
        const enum oeffis_event_type event = oeffis_get_event(m_oeffis);
        switch (event) {
        case OEFFIS_EVENT_NONE:
            return true;
        case OEFFIS_EVENT_CONNECTED_TO_EIS:
            if (!m_ei && !setupEi()) return false;
            break;
        case OEFFIS_EVENT_CLOSED:
            return fail("XDG RemoteDesktop portal session was cancelled or closed");
        case OEFFIS_EVENT_DISCONNECTED: {
            const char* message = oeffis_get_error_message(m_oeffis);
            return fail(std::string("XDG RemoteDesktop portal disconnected: ") +
                        (message ? message : "unknown error"));
        }
        }
    }
}

bool LibeiPortalSession::dispatchEi() {
    ei_dispatch(m_ei);
    while (ei_event* event = ei_get_event(m_ei)) {
        const enum ei_event_type type = ei_event_get_type(event);
        ei_device* device = ei_event_get_device(event);
        switch (type) {
        case EI_EVENT_CONNECT:
            m_eiConnected = true;
            break;
        case EI_EVENT_DISCONNECT:
            ei_event_unref(event);
            return fail("EIS disconnected the libei sender context");
        case EI_EVENT_SEAT_ADDED:
            ei_seat_bind_capabilities(
                ei_event_get_seat(event), EI_DEVICE_CAP_POINTER_ABSOLUTE,
                EI_DEVICE_CAP_BUTTON, EI_DEVICE_CAP_SCROLL, nullptr);
            break;
        case EI_EVENT_DEVICE_ADDED: {
            DeviceRecord record;
            record.device = ei_device_ref(device);
            if (ei_device_has_capability(
                    device, EI_DEVICE_CAP_POINTER_ABSOLUTE)) {
                for (std::size_t index = 0;; ++index) {
                    ei_region* region = ei_device_get_region(device, index);
                    if (!region) break;
                    record.regions.push_back({
                        ei_region_get_x(region), ei_region_get_y(region),
                        ei_region_get_width(region),
                        ei_region_get_height(region)});
                }
            }
            m_devices.push_back(std::move(record));
            break;
        }
        case EI_EVENT_DEVICE_RESUMED:
            if (DeviceRecord* record = find(device)) {
                record->resumed = true;
                if (!record->emulating) {
                    ei_device_start_emulating(device, ++m_sequence);
                    record->emulating = true;
                }
            }
            break;
        case EI_EVENT_DEVICE_PAUSED:
            if (DeviceRecord* record = find(device)) {
                record->resumed = false;
                record->emulating = false;
            }
            break;
        case EI_EVENT_DEVICE_REMOVED:
            remove(device);
            break;
        case EI_EVENT_SEAT_REMOVED:
        case EI_EVENT_FRAME:
        default:
            break;
        }
        ei_event_unref(event);
    }
    return true;
}

bool LibeiPortalSession::pump(int timeoutMs) {
    pollfd fds[2]{};
    fds[0].fd = oeffis_get_fd(m_oeffis);
    fds[0].events = POLLIN;
    int count = 1;
    if (m_ei) {
        fds[1].fd = ei_get_fd(m_ei);
        fds[1].events = POLLIN;
        count = 2;
    }
    int rc = 0;
    do {
        rc = ::poll(fds, count, timeoutMs);
    } while (rc < 0 && errno == EINTR);
    if (rc < 0)
        return fail(std::string("poll failed while processing portal/EIS: ") +
                    std::strerror(errno));
    if (rc == 0) return true;
    if (fds[0].revents & (POLLERR | POLLHUP | POLLNVAL))
        return fail("XDG RemoteDesktop portal event channel closed");
    if ((fds[0].revents & POLLIN) && !dispatchPortal()) return false;
    if (count == 2) {
        if (fds[1].revents & (POLLERR | POLLHUP | POLLNVAL))
            return fail("EIS event channel closed");
        if ((fds[1].revents & POLLIN) && !dispatchEi()) return false;
    }
    return true;
}

EisCapabilities LibeiPortalSession::capabilities() const {
    EisCapabilities result;
    for (const auto& record : m_devices) {
        if (!record.resumed) continue;
        result.absolutePointer = result.absolutePointer ||
            ei_device_has_capability(record.device,
                                     EI_DEVICE_CAP_POINTER_ABSOLUTE);
        result.button = result.button ||
            ei_device_has_capability(record.device, EI_DEVICE_CAP_BUTTON);
        result.scroll = result.scroll ||
            ei_device_has_capability(record.device, EI_DEVICE_CAP_SCROLL);
    }
    return result;
}

std::vector<EisRegion> LibeiPortalSession::regions() const {
    std::vector<EisRegion> result;
    for (const auto& record : m_devices) {
        if (record.resumed && ei_device_has_capability(
                                  record.device,
                                  EI_DEVICE_CAP_POINTER_ABSOLUTE)) {
            result.insert(result.end(), record.regions.begin(),
                          record.regions.end());
        }
    }
    return result;
}

bool LibeiPortalSession::ready() const {
    const EisCapabilities caps = capabilities();
    return m_eiConnected && caps.absolutePointer && caps.button &&
           caps.scroll && eisDesktopGeometry(regions()).has_value();
}

bool LibeiPortalSession::initialize() {
    teardown();
    m_lastError.clear();
    m_oeffis = oeffis_new(nullptr);
    if (!m_oeffis) return fail("oeffis_new failed");

    // Explicitly request pointer only. Start is asynchronous and is the only
    // operation here that may display the mandatory portal consent dialog.
    oeffis_create_session(m_oeffis, OEFFIS_DEVICE_POINTER);
    const auto portalDeadline = Clock::now() + std::chrono::seconds(120);
    auto eisDeadline = portalDeadline;
    while (Clock::now() < portalDeadline) {
        if (!pump(100)) {
            teardown();
            return false;
        }
        if (m_portalConnected && eisDeadline == portalDeadline)
            eisDeadline = Clock::now() + std::chrono::seconds(10);
        if (ready()) {
            m_initialized = true;
            m_lastError.clear();
            return true;
        }
        if (m_portalConnected && Clock::now() >= eisDeadline) {
            fail("EIS did not resume devices with absolute pointer, button, "
                 "scroll and valid regions");
            teardown();
            return false;
        }
    }
    fail("XDG RemoteDesktop portal permission request timed out");
    teardown();
    return false;
}

bool LibeiPortalSession::refresh() {
    if (!m_initialized) return fail("libei portal session is not initialized");
    return pump(0);
}

bool LibeiPortalSession::moveAbsolute(const DesktopPoint& point) {
    DeviceRecord* record = nullptr;
    for (auto& candidate : m_devices) {
        if (candidate.resumed && ei_device_has_capability(
                                     candidate.device,
                                     EI_DEVICE_CAP_POINTER_ABSOLUTE) &&
            pointInEisRegions(point, candidate.regions)) {
            record = &candidate;
            break;
        }
    }
    if (!record) return fail("no resumed EIS absolute pointer device");
    ei_device_pointer_motion_absolute(record->device, point.x, point.y);
    ei_device_frame(record->device, ei_now(m_ei));
    return true;
}

bool LibeiPortalSession::button(bool pressed) {
    DeviceRecord* record = active(EI_DEVICE_CAP_BUTTON);
    if (!record) return fail("no resumed EIS button device");
    ei_device_button_button(record->device, BTN_LEFT, pressed);
    ei_device_frame(record->device, ei_now(m_ei));
    return true;
}

bool LibeiPortalSession::scrollVertical(int logicalNotches) {
    const auto discrete = eisDiscreteVerticalScroll(logicalNotches);
    if (!discrete.has_value()) {
        return fail("logical scroll amount exceeds libei discrete range");
    }
    DeviceRecord* record = active(EI_DEVICE_CAP_SCROLL);
    if (!record) return fail("no resumed EIS scroll device");
    ei_device_scroll_discrete(record->device, 0, *discrete);
    ei_device_frame(record->device, ei_now(m_ei));
    return true;
}

void LibeiPortalSession::teardown() {
    for (auto& record : m_devices) {
        if (record.emulating && record.resumed)
            ei_device_stop_emulating(record.device);
        ei_device_unref(record.device);
    }
    m_devices.clear();
    if (m_ei) m_ei = ei_unref(m_ei);
    if (m_oeffis) m_oeffis = oeffis_unref(m_oeffis);
    m_portalConnected = false;
    m_eiConnected = false;
    m_initialized = false;
}

void LibeiPortalSession::shutdown() {
    teardown();
}

} // namespace

std::unique_ptr<IEisInputSession> createLibeiPortalSession() {
    return std::make_unique<LibeiPortalSession>();
}
