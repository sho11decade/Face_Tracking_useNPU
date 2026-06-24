#include "npu_vtube/filter/one_euro_filter.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace npuvt::filter {

OneEuroFilter::OneEuroFilter(Parameters parameters) : parameters_(parameters) {
}

void OneEuroFilter::reset() noexcept {
    has_last_value_ = false;
    last_value_ = 0.0;
    last_derivative_ = 0.0;
}

void OneEuroFilter::set_parameters(Parameters parameters) noexcept {
    parameters_ = parameters;
}

OneEuroFilter::Parameters OneEuroFilter::parameters() const noexcept {
    return parameters_;
}

double OneEuroFilter::compute_alpha(double cutoff, double delta_seconds) noexcept {
    if (cutoff <= 0.0 || delta_seconds <= 0.0) {
        return 1.0;
    }

    const double tau = 1.0 / (2.0 * std::numbers::pi * cutoff);
    return 1.0 / (1.0 + (tau / delta_seconds));
}

double OneEuroFilter::filter(double value, double delta_seconds) {
    if (!has_last_value_) {
        has_last_value_ = true;
        last_value_ = value;
        last_derivative_ = 0.0;
        return value;
    }

    const double alpha_derivative = compute_alpha(parameters_.derivative_cutoff, delta_seconds);
    const double derivative = (value - last_value_) / std::max(delta_seconds, 1e-6);
    last_derivative_ = (alpha_derivative * derivative) + ((1.0 - alpha_derivative) * last_derivative_);

    const double cutoff = parameters_.min_cutoff + (parameters_.beta * std::abs(last_derivative_));
    const double alpha = compute_alpha(cutoff, delta_seconds);
    last_value_ = (alpha * value) + ((1.0 - alpha) * last_value_);
    return last_value_;
}

}  // namespace npuvt::filter
