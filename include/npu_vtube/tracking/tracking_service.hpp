#pragma once

#include <chrono>

#include "npu_vtube/core/types.hpp"
#include "npu_vtube/filter/one_euro_filter.hpp"

namespace npuvt::tracking {

struct TrackingOptions {
    npuvt::filter::OneEuroFilter::Parameters center_parameters {};
    npuvt::filter::OneEuroFilter::Parameters scale_parameters {};
    npuvt::filter::OneEuroFilter::Parameters angle_parameters {};
};

class TrackingService {
public:
    TrackingService();
    explicit TrackingService(TrackingOptions options);

    void set_options(TrackingOptions options);
    [[nodiscard]] TrackingOptions options() const;
    void reset();

    [[nodiscard]] npuvt::core::TrackingState update(const npuvt::core::FaceObservation& observation);

private:
    TrackingOptions options_ {};
    npuvt::filter::OneEuroFilter center_x_filter_;
    npuvt::filter::OneEuroFilter center_y_filter_;
    npuvt::filter::OneEuroFilter scale_filter_;
    npuvt::filter::OneEuroFilter yaw_filter_;
    npuvt::filter::OneEuroFilter pitch_filter_;
    npuvt::filter::OneEuroFilter roll_filter_;
    std::chrono::steady_clock::time_point last_timestamp_ {};
    bool has_last_timestamp_ {false};
};

}  // namespace npuvt::tracking
