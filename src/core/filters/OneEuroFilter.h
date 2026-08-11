#ifndef ONEEUROFILTER_H
#define ONEEUROFILTER_H

#include <cmath>
#include "src/core/gestures/Landmarks.h"

class LowPassFilter {
public:
    LowPassFilter(double alpha = 1.0) : m_alpha(alpha), m_initialized(false), m_hatx(0.0) {}

    double filter(double x) {
        if (!m_initialized) {
            m_hatx = x;
            m_initialized = true;
        } else {
            m_hatx = m_alpha * x + (1.0 - m_alpha) * m_hatx;
        }
        return m_hatx;
    }

    void setAlpha(double alpha) { m_alpha = alpha; }
    double lastValue() const { return m_hatx; }

private:
    double m_alpha;
    bool m_initialized;
    double m_hatx;
};

class OneEuroFilter {
public:
    OneEuroFilter(double freq = 30.0, double minCutoff = 1.0, double beta = 0.007, double dCutoff = 1.0)
        : m_freq(freq), m_minCutoff(minCutoff), m_beta(beta), m_dCutoff(dCutoff),
          m_xFilter(1.0), m_dxFilter(1.0), m_lastTime(-1.0) {}

    double filter(double x, double timestamp = -1.0) {
        if (m_lastTime != -1.0 && timestamp != -1.0) {
            double dt = timestamp - m_lastTime;
            if (dt > 0.0) m_freq = 1.0 / dt;
        }
        m_lastTime = timestamp;

        double prevX = m_xFilter.lastValue();
        double dx = (m_xFilter.lastValue() == 0.0) ? 0.0 : (x - prevX) * m_freq;
        double edx = m_dxFilter.filter(dx);

        double cutoff = m_minCutoff + m_beta * std::abs(edx);
        m_xFilter.setAlpha(alpha(cutoff));

        return m_xFilter.filter(x);
    }

    Point3D filterPoint(const Point3D& pt, double timestamp = -1.0) {
        return Point3D(
            filter(pt.x, timestamp),
            filter(pt.y, timestamp),
            filter(pt.z, timestamp)
        );
    }

private:
    double alpha(double cutoff) const {
        double tau = 1.0 / (2.0 * M_PI * cutoff);
        double te = 1.0 / m_freq;
        return 1.0 / (1.0 + tau / te);
    }

    double m_freq;
    double m_minCutoff;
    double m_beta;
    double m_dCutoff;
    LowPassFilter m_xFilter;
    LowPassFilter m_dxFilter;
    double m_lastTime;
};

#endif // ONEEUROFILTER_H
