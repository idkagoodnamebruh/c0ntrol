#ifndef IEISINPUTSESSION_H
#define IEISINPUTSESSION_H

#include <memory>
#include <string>
#include <vector>

#include "src/platform/linux/EisRegion.h"

struct EisCapabilities {
    bool absolutePointer{false};
    bool button{false};
    bool scroll{false};
};

class IEisInputSession {
public:
    virtual ~IEisInputSession() = default;

    virtual bool initialize() = 0;
    virtual bool refresh() = 0;
    virtual EisCapabilities capabilities() const = 0;
    virtual std::vector<EisRegion> regions() const = 0;
    virtual bool moveAbsolute(const DesktopPoint& point) = 0;
    virtual bool button(bool pressed) = 0;
    virtual bool scrollVertical(int logicalNotches) = 0;
    virtual void shutdown() = 0;
    virtual std::string lastError() const = 0;
};

std::unique_ptr<IEisInputSession> createLibeiPortalSession();

#endif // IEISINPUTSESSION_H
