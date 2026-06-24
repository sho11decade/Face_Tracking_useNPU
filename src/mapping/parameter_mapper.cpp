#include "npu_vtube/mapping/parameter_mapper.hpp"

#include <algorithm>

namespace npuvt::mapping {

float clamp_unit(float value) {
    return std::clamp(value, -1.0F, 1.0F);
}

ParameterMapper::ParameterMapper() = default;

ParameterMapper::ParameterMapper(MappingOptions options) : options_(options) {
}

void ParameterMapper::set_options(MappingOptions options) {
    options_ = options;
}

MappingOptions ParameterMapper::options() const {
    return options_;
}

npuvt::core::MappedParameters ParameterMapper::map(const npuvt::core::TrackingState& state) const {
    const float half_width = state.frame_width > 0 ? static_cast<float>(state.frame_width) * 0.5F : 640.0F;
    const float half_height = state.frame_height > 0 ? static_cast<float>(state.frame_height) * 0.5F : 360.0F;
    const float normalized_x = clamp_unit((state.face_center.x - half_width) / std::max(half_width, 1.0F));
    const float normalized_y = clamp_unit((state.face_center.y - half_height) / std::max(half_height, 1.0F));
    const float scale_reference = std::max(std::min(half_width, half_height) * options_.scale_reference_ratio, 1.0F);
    const float normalized_scale = clamp_unit(state.face_scale / scale_reference);

    return {
        .head_x = normalized_x,
        .head_y = normalized_y,
        .head_z = normalized_scale,
        .angle_yaw = state.yaw,
        .angle_pitch = state.pitch,
        .angle_roll = state.roll,
    };
}

}  // namespace npuvt::mapping
