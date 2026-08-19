#ifndef HANDFEATURES_H
#define HANDFEATURES_H

#include <limits>

#include "src/core/tracking/HandTrackingTypes.h"

struct HandFeatures {
    bool valid{false};
    double handScale{0.0};
    Point3D palmCenter{};
    Point3D pointerPoint{};
    double pinchRatio{std::numeric_limits<double>::infinity()};

    double thumbCurl{0.0};
    double indexCurl{0.0};
    double middleCurl{0.0};
    double ringCurl{0.0};
    double pinkyCurl{0.0};
    double thumbSpreadRatio{0.0};

    bool thumbExtended{false};
    bool indexExtended{false};
    bool middleExtended{false};
    bool ringExtended{false};
    bool pinkyExtended{false};

    Handedness handedness{Handedness::UNKNOWN};
};

#endif // HANDFEATURES_H
