#ifndef WINDOWSSYSTEMINPUTBACKEND_H
#define WINDOWSSYSTEMINPUTBACKEND_H

#include "src/core/actions/ISystemInputBackend.h"

class WindowsSystemInputBackend final : public ISystemInputBackend {
public:
    bool initialize() override;
    DesktopGeometry desktopGeometry() const override { return m_desktop; }
    bool movePointer(const DesktopPoint& point) override;
    bool primaryButtonDown() override;
    bool primaryButtonUp() override;
    void shutdown() override;
    std::string lastError() const override { return m_lastError; }

private:
    bool sendMouse(unsigned long flags, long x = 0, long y = 0);

    DesktopGeometry m_desktop{};
    bool m_initialized{false};
    std::string m_lastError;
};

#endif // WINDOWSSYSTEMINPUTBACKEND_H
