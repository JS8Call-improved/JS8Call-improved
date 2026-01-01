#include "kalman.h"

#include <algorithm>
#include <cmath>

namespace js8 {
void FrequencyTracker::reset(double initial_hz, double sample_rate_hz,
                             double alpha, double max_step_hz,
                             double max_error_hz) {
    m_enabled = true;
    m_est_hz = initial_hz;
    m_fs = sample_rate_hz;
    m_alpha = alpha;
    m_max_step_hz = max_step_hz;
    m_max_error_hz = max_error_hz;
    m_sum_abs = 0.0;
    m_updates = 0;
}

void FrequencyTracker::disable() { m_enabled = false; }

bool FrequencyTracker::enabled() const noexcept { return m_enabled; }

double FrequencyTracker::currentHz() const noexcept { return m_est_hz; }

double FrequencyTracker::averageStepHz() const noexcept {
    return m_updates > 0 ? m_sum_abs / static_cast<double>(m_updates) : 0.0;
}

void FrequencyTracker::apply(std::complex<float> *data, int count) const {
    if (!m_enabled || !data || count <= 0 || m_fs <= 0.0)
        return;

    double const dphi = 2.0 * std::numbers::pi * (m_est_hz / m_fs);
    auto const wstep = std::polar(1.0f, static_cast<float>(dphi));
    auto w = std::complex<float>{1.0f, 0.0f};

    for (int i = 0; i < count; ++i) {
        w *= wstep;
        data[i] *= w;
    }
}

void FrequencyTracker::update(double residual_hz, double weight) {
    if (!m_enabled || m_fs <= 0.0)
        return;
    if (!std::isfinite(residual_hz) || !std::isfinite(weight) || weight <= 0.0)
        return;
    if (std::abs(residual_hz) > m_max_error_hz)
        return;

    residual_hz *= std::min(weight, 1.0);

    double const step = std::clamp(residual_hz, -m_max_step_hz, m_max_step_hz);
    m_est_hz += m_alpha * step;

    m_sum_abs += std::abs(step);
    ++m_updates;
}

void TimingTracker::reset(double initial_samples, double alpha, double max_step,
                          double max_total_error) {
    m_enabled = true;
    m_est_samples = initial_samples;
    m_alpha = alpha;
    m_max_step = max_step;
    m_max_total = max_total_error;
    m_sum_abs = 0.0;
    m_updates = 0;
}

void TimingTracker::disable() { m_enabled = false; }

bool TimingTracker::enabled() const noexcept { return m_enabled; }

double TimingTracker::currentSamples() const noexcept { return m_est_samples; }

double TimingTracker::averageStepSamples() const noexcept {
    return m_updates > 0 ? m_sum_abs / static_cast<double>(m_updates) : 0.0;
}

void TimingTracker::update(double residual_samples, double weight) {
    if (!m_enabled)
        return;
    if (!std::isfinite(residual_samples) || !std::isfinite(weight) ||
        weight <= 0.0)
        return;

    residual_samples *= std::min(weight, 1.0);

    double const step = std::clamp(residual_samples, -m_max_step, m_max_step);
    double const next = m_est_samples + m_alpha * step;

    if (std::abs(next) > m_max_total)
        return;

    m_est_samples = next;
    m_sum_abs += std::abs(step);
    ++m_updates;
}
} // namespace js8
