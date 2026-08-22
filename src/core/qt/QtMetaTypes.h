#ifndef QTMETATYPES_H
#define QTMETATYPES_H

#include <QMetaType>

#include "src/core/gestures/Landmarks.h"
#include "src/core/metrics/PipelineMetrics.h"
#include "src/core/tracking/HandTrackingTypes.h"

// Landmarks crosses from VisionWorker's thread to the GUI through a queued
// connection. Keep this Qt declaration out of the core-only Landmarks header.
Q_DECLARE_METATYPE(Landmarks)
Q_DECLARE_METATYPE(HandTrackingFrame)
Q_DECLARE_METATYPE(PipelineMetrics)

#endif // QTMETATYPES_H
