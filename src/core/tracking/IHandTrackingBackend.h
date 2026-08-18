#ifndef IHANDTRACKINGBACKEND_H
#define IHANDTRACKINGBACKEND_H

#include <string>

#include "src/core/tracking/HandTrackingTypes.h"

class IHandTrackingBackend {
public:
    virtual ~IHandTrackingBackend() = default;
    virtual bool initialize(const HandTrackingConfig& config) = 0;
    virtual HandTrackingFrame process(const RgbImageView& image,
                                      std::int64_t timestampUs,
                                      std::uint64_t frameId) = 0;
    virtual void shutdown() = 0;
    virtual std::string lastError() const = 0;
};

#endif // IHANDTRACKINGBACKEND_H
