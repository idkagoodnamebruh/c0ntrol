#ifndef DYNAMICGESTURETRACKER_H
#define DYNAMICGESTURETRACKER_H

#include <vector>
#include <chrono>
#include "src/core/gestures/Landmarks.h"

enum class DynamicGesture {
    NONE,
    SWIPE_LEFT,
    SWIPE_RIGHT,
    SWIPE_UP,
    SWIPE_DOWN
};

class DynamicGestureTracker {
public:
    DynamicGestureTracker(int maxHistory = 10, double swipeThreshold = 0.15)
        : m_maxHistory(maxHistory), m_swipeThreshold(swipeThreshold) {}

    void addFrame(const Landmarks& landmarks) {
        if (landmarks.points.empty()) return;
        
        Point3D wrist = landmarks.points[0];
        m_history.push_back(wrist);

        if (m_history.size() > m_maxHistory) {
            m_history.erase(m_history.begin());
        }
    }

    DynamicGesture detectGesture() {
        if (m_history.size() < m_maxHistory) return DynamicGesture::NONE;

        Point3D start = m_history.front();
        Point3D end = m_history.back();

        double dx = end.x - start.x;
        double dy = end.y - start.y;

        if (std::abs(dx) > m_swipeThreshold && std::abs(dx) > std::abs(dy)) {
            m_history.clear();
            return (dx > 0) ? DynamicGesture::SWIPE_RIGHT : DynamicGesture::SWIPE_LEFT;
        }

        if (std::abs(dy) > m_swipeThreshold && std::abs(dy) > std::abs(dx)) {
            m_history.clear();
            return (dy > 0) ? DynamicGesture::SWIPE_DOWN : DynamicGesture::SWIPE_UP;
        }

        return DynamicGesture::NONE;
    }

    void reset() {
        m_history.clear();
    }

private:
    std::size_t m_maxHistory;
    double m_swipeThreshold;
    std::vector<Point3D> m_history;
};

#endif // DYNAMICGESTURETRACKER_H
