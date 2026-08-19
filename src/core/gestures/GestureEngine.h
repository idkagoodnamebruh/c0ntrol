#ifndef GESTUREENGINE_H
#define GESTUREENGINE_H

#include "src/core/gestures/GestureTypes.h"
#include "src/core/gestures/HandFeatures.h"

class GestureEngine {
public:
    explicit GestureEngine(GestureConfig config = {});

    GestureObservation observe(const HandFeatures& features,
                               std::uint64_t frameId,
                               std::int64_t timestampUs) const;

    const GestureConfig& config() const { return m_config; }

private:
    GestureConfig m_config;
};

#endif // GESTUREENGINE_H
