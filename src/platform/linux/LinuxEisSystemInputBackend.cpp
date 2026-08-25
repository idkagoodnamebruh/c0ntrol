#include "LinuxEisSystemInputBackend.h"

#include <utility>

LinuxEisSystemInputBackend::LinuxEisSystemInputBackend(
    std::unique_ptr<IEisInputSession> session)
    : m_session(std::move(session)) {}

LinuxEisSystemInputBackend::~LinuxEisSystemInputBackend() {
    shutdown();
}

bool LinuxEisSystemInputBackend::fail(const std::string& error) {
    m_lastError = error;
    return false;
}

bool LinuxEisSystemInputBackend::validateReady() {
    const EisCapabilities capabilities = m_session->capabilities();
    if (!capabilities.absolutePointer)
        return fail("EIS does not provide an active absolute pointer device");
    if (!capabilities.button)
        return fail("EIS does not provide an active button device");
    if (!capabilities.scroll)
        return fail("EIS does not provide an active scroll device");

    m_regions = m_session->regions();
    const auto geometry = eisDesktopGeometry(m_regions);
    if (!geometry.has_value())
        return fail("EIS returned no valid overflow-safe desktop regions");
    m_desktop = *geometry;
    return true;
}

bool LinuxEisSystemInputBackend::initialize() {
    if (m_initialized) return true;
    if (!m_session) return fail("Linux EIS session is unavailable");
    if (!m_session->initialize()) return fail(m_session->lastError());
    if (!validateReady()) {
        m_session->shutdown();
        return false;
    }
    m_initialized = true;
    m_primaryButtonDownOwned = false;
    m_lastError.clear();
    return true;
}

bool LinuxEisSystemInputBackend::refresh(const char* operation) {
    if (!m_initialized)
        return fail("Linux EIS input backend is not initialized");
    if (!m_session->refresh())
        return fail(m_session->lastError().empty()
                        ? std::string("EIS disconnected during ") + operation
                        : m_session->lastError());
    return validateReady();
}

bool LinuxEisSystemInputBackend::movePointer(const DesktopPoint& point) {
    if (!refresh("absolute pointer motion")) return false;
    if (!pointInEisRegions(point, m_regions))
        return fail("absolute pointer point is outside all EIS regions");
    if (!m_session->moveAbsolute(point)) return fail(m_session->lastError());
    m_lastError.clear();
    return true;
}

bool LinuxEisSystemInputBackend::primaryButtonDown() {
    if (m_primaryButtonDownOwned) return true;
    if (!refresh("primary button down")) return false;
    if (!m_session->button(true)) return fail(m_session->lastError());
    m_primaryButtonDownOwned = true;
    m_lastError.clear();
    return true;
}

bool LinuxEisSystemInputBackend::primaryButtonUp() {
    if (!m_primaryButtonDownOwned) return true;
    if (!refresh("primary button up")) return false;
    if (!m_session->button(false)) return fail(m_session->lastError());
    m_primaryButtonDownOwned = false;
    m_lastError.clear();
    return true;
}

bool LinuxEisSystemInputBackend::scrollVertical(int notches) {
    if (notches == 0) return fail("vertical scroll amount must be non-zero");
    if (!refresh("vertical scroll")) return false;
    if (!m_session->scrollVertical(notches))
        return fail(m_session->lastError());
    m_lastError.clear();
    return true;
}

void LinuxEisSystemInputBackend::shutdown() {
    if (!m_session) return;
    if (m_initialized && m_primaryButtonDownOwned) {
        if (m_session->refresh() && m_session->button(false)) {
            m_primaryButtonDownOwned = false;
        } else if (m_lastError.empty()) {
            m_lastError = m_session->lastError();
        }
    }
    if (m_initialized) m_session->shutdown();
    m_initialized = false;
    m_primaryButtonDownOwned = false;
    m_regions.clear();
    m_desktop = {};
}
