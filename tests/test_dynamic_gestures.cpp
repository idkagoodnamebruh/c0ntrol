#include <iostream>
#include <cassert>
#include "src/core/gestures/DynamicGestureTracker.h"

void testSwipeDetection() {
    DynamicGestureTracker tracker(5, 0.10);

    Landmarks lm1;
    lm1.points.push_back(Point3D(0.1, 0.5, 0.0));
    tracker.addFrame(lm1);

    Landmarks lm2;
    lm2.points.push_back(Point3D(0.2, 0.5, 0.0));
    tracker.addFrame(lm2);

    Landmarks lm3;
    lm3.points.push_back(Point3D(0.3, 0.5, 0.0));
    tracker.addFrame(lm3);

    Landmarks lm4;
    lm4.points.push_back(Point3D(0.4, 0.5, 0.0));
    tracker.addFrame(lm4);

    Landmarks lm5;
    lm5.points.push_back(Point3D(0.5, 0.5, 0.0));
    tracker.addFrame(lm5);

    DynamicGesture gesture = tracker.detectGesture();
    assert(gesture == DynamicGesture::SWIPE_RIGHT);

    std::cout << "[PASS] testSwipeDetection" << std::endl;
}

int main() {
    testSwipeDetection();
    return 0;
}
