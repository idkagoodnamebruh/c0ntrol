#ifndef ONEEUROFILTER_H
#define ONEEUROFILTER_H

#include <algorithm>
#include <cmath>

struct OneEuroConfig {
    double minCutoff{1.0};
    double beta{0.05};
    double derivativeCutoff{1.0};
    double maxDeltaSeconds{1.0};
};

// A One Euro filter for exactly one scalar channel. Timestamps are seconds.
class OneEuroFilter {
public:
    OneEuroFilter() { configure({}); }
    explicit OneEuroFilter(OneEuroConfig config) { configure(config); }

    void configure(OneEuroConfig config) {
        m_config.minCutoff = positiveOr(config.minCutoff, 1.0);
        m_config.beta = nonNegativeOr(config.beta, 0.0);
        m_config.derivativeCutoff = positiveOr(config.derivativeCutoff, 1.0);
        m_config.maxDeltaSeconds = positiveOr(config.maxDeltaSeconds, 1.0);
        reset();
    }

    double filter(double value, double timestampSeconds) {
        if (!std::isfinite(value)) {
            return m_signal.initialized() ? m_signal.lastValue() : 0.0;
        }

        if (!std::isfinite(timestampSeconds)) {
            reset();
            return value;
        }

        if (!m_initialized) {
            return initialize(value, timestampSeconds);
        }

        const double dt = timestampSeconds - m_previousTimestampSeconds;
        if (!std::isfinite(dt) || dt <= 0.0 || dt > m_config.maxDeltaSeconds) {
            reset();
            return initialize(value, timestampSeconds);
        }

        const double derivative = (value - m_previousRawValue) / dt;
        const double filteredDerivative =
            m_derivative.filter(derivative, alpha(m_config.derivativeCutoff, dt));
        const double cutoff =
            m_config.minCutoff + m_config.beta * std::abs(filteredDerivative);
        const double filteredValue = m_signal.filter(value, alpha(cutoff, dt));

        m_previousRawValue = value;
        m_previousTimestampSeconds = timestampSeconds;
        return filteredValue;
    }

    void reset() {
        m_signal.reset();
        m_derivative.reset();
        m_previousRawValue = 0.0;
        m_previousTimestampSeconds = 0.0;
        m_initialized = false;
    }

    bool initialized() const { return m_initialized; }
    const OneEuroConfig& config() const { return m_config; }

private:
    class LowPassFilter {
    public:
        double filter(double value, double alphaValue) {
            const double boundedAlpha = std::clamp(alphaValue, 0.0, 1.0);
            if (!m_initialized) {
                m_value = value;
                m_initialized = true;
            } else {
                m_value = boundedAlpha * value + (1.0 - boundedAlpha) * m_value;
            }
            return m_value;
        }

        void initialize(double value) {
            m_value = value;
            m_initialized = true;
        }

        void reset() {
            m_value = 0.0;
            m_initialized = false;
        }

        bool initialized() const { return m_initialized; }
        double lastValue() const { return m_value; }

    private:
        double m_value{0.0};
        bool m_initialized{false};
    };

    static double positiveOr(double value, double fallback) {
        return std::isfinite(value) && value > 0.0 ? value : fallback;
    }

    static double nonNegativeOr(double value, double fallback) {
        return std::isfinite(value) && value >= 0.0 ? value : fallback;
    }

    static double alpha(double cutoff, double dt) {
        constexpr double pi = 3.14159265358979323846;
        const double tau = 1.0 / (2.0 * pi * cutoff);
        return 1.0 / (1.0 + tau / dt);
    }

    double initialize(double value, double timestampSeconds) {
        m_signal.initialize(value);
        m_derivative.initialize(0.0);
        m_previousRawValue = value;
        m_previousTimestampSeconds = timestampSeconds;
        m_initialized = true;
        return value;
    }

    OneEuroConfig m_config{};
    LowPassFilter m_signal;
    LowPassFilter m_derivative;
    double m_previousRawValue{0.0};
    double m_previousTimestampSeconds{0.0};
    bool m_initialized{false};
};

#endif // ONEEUROFILTER_H
