#ifndef NULLSYSTEMINPUTBACKEND_H
#define NULLSYSTEMINPUTBACKEND_H

#include "src/core/actions/ISystemInputBackend.h"

class NullSystemInputBackend final : public ISystemInputBackend {
public:
    bool initialize() override;
    DesktopGeometry desktopGeometry() const override { return {}; }
    bool movePointer(const DesktopPoint&) override;
    bool primaryButtonDown() override;
    bool primaryButtonUp() override;
    void shutdown() override;
    std::string lastError() const override { return m_lastError; }

private:
    std::string m_lastError{"native input is unsupported on this platform"};
};

#endif // NULLSYSTEMINPUTBACKEND_H
