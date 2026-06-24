#include "npu_vtube/tracking/tracking_service.hpp"

namespace npuvt::tracking {

npuvt::core::TrackingState TrackingService::update(const npuvt::core::FaceObservation& observation) const {
    const auto center_x = observation.face_bbox.x + (observation.face_bbox.width * 0.5F);
    const auto center_y = observation.face_bbox.y + (observation.face_bbox.height * 0.5F);

    return {
        .frame_id = observation.frame_id,
        .timestamp = observation.timestamp,
        .valid = observation.detected,
        .face_center = {.x = center_x, .y = center_y},
        .face_scale = observation.face_bbox.width,
        .yaw = observation.yaw,
        .pitch = observation.pitch,
        .roll = observation.roll,
    };
}

}  // namespace npuvt::tracking
