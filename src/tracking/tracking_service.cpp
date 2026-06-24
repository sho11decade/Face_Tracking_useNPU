#include "npu_vtube/tracking/tracking_service.hpp"

#include <algorithm>

namespace npuvt::tracking {

namespace {

double to_seconds(const std::chrono::steady_clock::duration duration) {
    return std::chrono::duration<double>(duration).count();
}

float clamp_unit(float value) {
    return std::clamp(value, -1.0F, 1.0F);
}

}  // namespace

TrackingService::TrackingService() {
    set_options(TrackingOptions {});
}

TrackingService::TrackingService(TrackingOptions options) {
    set_options(options);
}

void TrackingService::set_options(TrackingOptions options) {
    options_ = options;
    center_x_filter_.set_parameters(options_.center_parameters);
    center_y_filter_.set_parameters(options_.center_parameters);
    scale_filter_.set_parameters(options_.scale_parameters);
    yaw_filter_.set_parameters(options_.angle_parameters);
    pitch_filter_.set_parameters(options_.angle_parameters);
    roll_filter_.set_parameters(options_.angle_parameters);
}

TrackingOptions TrackingService::options() const {
    return options_;
}

void TrackingService::reset() {
    center_x_filter_.reset();
    center_y_filter_.reset();
    scale_filter_.reset();
    yaw_filter_.reset();
    pitch_filter_.reset();
    roll_filter_.reset();
    has_last_timestamp_ = false;
    last_timestamp_ = {};
}

npuvt::core::TrackingState TrackingService::update(const npuvt::core::FaceObservation& observation) {
    const auto center_x = observation.face_bbox.x + (observation.face_bbox.width * 0.5F);
    const auto center_y = observation.face_bbox.y + (observation.face_bbox.height * 0.5F);

    double delta_seconds = 1.0 / 60.0;
    if (has_last_timestamp_) {
        delta_seconds = std::max(to_seconds(observation.timestamp - last_timestamp_), 1e-6);
    }
    last_timestamp_ = observation.timestamp;
    has_last_timestamp_ = true;

    const auto filtered_center_x = static_cast<float>(center_x_filter_.filter(center_x, delta_seconds));
    const auto filtered_center_y = static_cast<float>(center_y_filter_.filter(center_y, delta_seconds));
    const auto filtered_scale = static_cast<float>(scale_filter_.filter(observation.face_bbox.width, delta_seconds));

    const auto filtered_yaw = static_cast<float>(yaw_filter_.filter(observation.yaw, delta_seconds));
    const auto filtered_pitch = static_cast<float>(pitch_filter_.filter(observation.pitch, delta_seconds));
    const auto filtered_roll = static_cast<float>(roll_filter_.filter(observation.roll, delta_seconds));

    return {
        .frame_id = observation.frame_id,
        .timestamp = observation.timestamp,
        .frame_width = observation.frame_width,
        .frame_height = observation.frame_height,
        .valid = observation.detected,
        .face_center = {.x = filtered_center_x, .y = filtered_center_y},
        .face_scale = filtered_scale,
        .yaw = filtered_yaw,
        .pitch = filtered_pitch,
        .roll = filtered_roll,
        .confidence = observation.confidence,
    };
}

}  // namespace npuvt::tracking
