#ifndef GESTURECLASSIFIER_H
#define GESTURECLASSIFIER_H

#include "Landmarks.h"

enum class GestureType {
    NONE,
    UNKNOWN,
    POINTING,
    PINCH,
    PALM_OPEN,
    FIST,
    VICTORY
};

class GestureClassifier {
public:
    static GestureType classify(const Landmarks& landmarks);
};

#endif // GESTURECLASSIFIER_H
