#ifndef LINUXEISSYSTEMINPUTBACKEND_H
#define LINUXEISSYSTEMINPUTBACKEND_H

#include <memory>
#include <string>
#include <vector>

#include "src/core/actions/ISystemInputBackend.h"
#include "src/platform/linux/IEisInputSession.h"

class LinuxEisSystemInputBackend final : public ISystemInputBackend {
public:
    explicit LinuxEisSystemInputBackend(
        std::unique_ptr<IEisInputSession> session);
    ~LinuxEisSystemInputBackend() override;

    bool initialize() override;
    DesktopGeometry desktopGeometry() const override { return m_desktop; }
    bool movePointer(const DesktopPoint& point) override;
    bool primaryButtonDown() override;
    bool primaryButtonUp() override;
    bool scrollVertical(int notches) override;
    void shutdown() override;
    std::string lastError() const override { return m_lastError; }

private:
    bool refresh(const char* operation);
    bool validateReady();
    bool fail(const std::string& error);

    std::unique_ptr<IEisInputSession> m_session;
    std::vector<EisRegion> m_regions;
    DesktopGeometry m_desktop{};
    bool m_initialized{false};
    bool m_primaryButtonDownOwned{false};
    std::string m_lastError;
};

#endif // LINUXEISSYSTEMINPUTBACKEND_H
