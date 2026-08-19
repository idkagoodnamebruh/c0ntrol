#ifndef HANDFEATUREEXTRACTOR_H
#define HANDFEATUREEXTRACTOR_H

#include "src/core/gestures/GestureTypes.h"
#include "src/core/gestures/HandFeatures.h"

class HandFeatureExtractor {
public:
    explicit HandFeatureExtractor(GestureConfig config = {});

    HandFeatures extract(const TrackedHand& hand) const;
    static double computeHandScale(const std::array<Point3D, 21>& landmarks);

    const GestureConfig& config() const { return m_config; }

private:
    double fingerCurl(const std::array<Point3D, 21>& points,
                      std::size_t mcp, std::size_t pip, std::size_t dip,
                      std::size_t tip, double scale, bool& valid) const;

    GestureConfig m_config;
};

#endif // HANDFEATUREEXTRACTOR_H
