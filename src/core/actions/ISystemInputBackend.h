#ifndef ISYSTEMINPUTBACKEND_H
#define ISYSTEMINPUTBACKEND_H

#include <string>

#include "src/core/actions/ActionTypes.h"

class ISystemInputBackend {
public:
    virtual ~ISystemInputBackend() = default;

    virtual bool initialize() = 0;
    virtual DesktopGeometry desktopGeometry() const = 0;
    virtual bool movePointer(const DesktopPoint& point) = 0;
    virtual bool primaryButtonDown() = 0;
    virtual bool primaryButtonUp() = 0;
    virtual void shutdown() = 0;
    virtual std::string lastError() const = 0;
};

#endif // ISYSTEMINPUTBACKEND_H
