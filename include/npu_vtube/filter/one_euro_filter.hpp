#pragma once

namespace npuvt::filter {

class OneEuroFilter {
public:
    struct Parameters {
        double min_cutoff {1.0};
        double beta {0.0};
        double derivative_cutoff {1.0};
    };

    explicit OneEuroFilter(Parameters parameters = {});

    void reset() noexcept;
    void set_parameters(Parameters parameters) noexcept;
    [[nodiscard]] Parameters parameters() const noexcept;
    [[nodiscard]] double filter(double value, double delta_seconds);

private:
    static double compute_alpha(double cutoff, double delta_seconds) noexcept;

    Parameters parameters_ {};
    bool has_last_value_ {false};
    double last_value_ {0.0};
    double last_derivative_ {0.0};
};

}  // namespace npuvt::filter
