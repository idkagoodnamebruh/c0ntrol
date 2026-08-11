#include "GestureClassifier.h"

GestureType GestureClassifier::classify(const Landmarks& landmarks) {
    if (landmarks.points.size() < 21) {
        return GestureType::NONE;
    }

    const auto& pts = landmarks.points;

    // Distancia entre pulgar (4) e índice (8) para clic (PINCH)
    double pinchDist = pts[4].distanceTo(pts[8]);
    if (pinchDist < 0.05) {
        return GestureType::PINCH;
    }

    // Comprobaciones de extensión de dedos
    bool indexExtended = pts[8].y < pts[6].y;
    bool middleExtended = pts[12].y < pts[10].y;
    bool ringExtended = pts[16].y < pts[14].y;
    bool pinkyExtended = pts[20].y < pts[18].y;

    if (indexExtended && !middleExtended && !ringExtended && !pinkyExtended) {
        return GestureType::POINTING;
    }

    if (indexExtended && middleExtended && !ringExtended && !pinkyExtended) {
        return GestureType::VICTORY;
    }

    if (indexExtended && middleExtended && ringExtended && pinkyExtended) {
        return GestureType::PALM_OPEN;
    }

    if (!indexExtended && !middleExtended && !ringExtended && !pinkyExtended) {
        return GestureType::FIST;
    }

    return GestureType::UNKNOWN;
}
