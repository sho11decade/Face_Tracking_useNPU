#include "npu_vtube/mapping/parameter_mapper.hpp"

namespace npuvt::mapping {

npuvt::core::MappedParameters ParameterMapper::map(const npuvt::core::TrackingState& state) const {
    return {
        .head_x = state.face_center.x,
        .head_y = state.face_center.y,
        .head_z = state.face_scale,
        .angle_yaw = state.yaw,
        .angle_pitch = state.pitch,
        .angle_roll = state.roll,
    };
}

}  // namespace npuvt::mapping
