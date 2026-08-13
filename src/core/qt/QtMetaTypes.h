#ifndef QTMETATYPES_H
#define QTMETATYPES_H

#include <QMetaType>

#include "src/core/gestures/Landmarks.h"

// Landmarks crosses from VisionWorker's thread to the GUI through a queued
// connection. Keep this Qt declaration out of the core-only Landmarks header.
Q_DECLARE_METATYPE(Landmarks)

#endif // QTMETATYPES_H
