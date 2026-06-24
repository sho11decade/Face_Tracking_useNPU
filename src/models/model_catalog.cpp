#include "npu_vtube/models/model_catalog.hpp"

#include <utility>

namespace npuvt::models {

namespace {

std::vector<ModelDefinition> make_defaults() {
    return {
        {
            .role = ModelRole::face_detection,
            .model_id = "face-detection-0200",
            .display_name = "Face Detection 0200",
            .description = "Primary low-latency face detector for front-facing cameras.",
            .input_width = 256,
            .input_height = 256,
            .input_channels = 3,
            .input_color_order = "BGR",
            .recommended_for_npu = true,
        },
        {
            .role = ModelRole::facial_landmarks,
            .model_id = "landmarks-regression-retail-0009",
            .display_name = "Facial Landmarks Regression 0009",
            .description = "Lightweight 5-point landmarks regressor for tight face crops.",
            .input_width = 48,
            .input_height = 48,
            .input_channels = 3,
            .input_color_order = "BGR",
            .recommended_for_npu = true,
        },
        {
            .role = ModelRole::head_pose,
            .model_id = "head-pose-estimation-adas-0001",
            .display_name = "Head Pose Estimation 0001",
            .description = "Direct yaw/pitch/roll regression for VTuber head pose.",
            .input_width = 60,
            .input_height = 60,
            .input_channels = 3,
            .input_color_order = "BGR",
            .recommended_for_npu = true,
        },
        {
            .role = ModelRole::facial_landmarks_35,
            .model_id = "facial-landmarks-35-adas-0002",
            .display_name = "Facial Landmarks 35 0002",
            .description = "Higher-density facial landmark model for future expression control.",
            .input_width = 60,
            .input_height = 60,
            .input_channels = 3,
            .input_color_order = "BGR",
            .recommended_for_npu = false,
        },
    };
}

}  // namespace

const std::vector<ModelDefinition>& ModelCatalog::defaults() const {
    if (defaults_.empty()) {
        defaults_ = make_defaults();
    }

    return defaults_;
}

const ModelDefinition* ModelCatalog::find(ModelRole role) const {
    const auto& default_models = defaults();
    for (const auto& model : default_models) {
        if (model.role == role) {
            return &model;
        }
    }

    return nullptr;
}

std::string_view to_string(ModelRole role) noexcept {
    switch (role) {
    case ModelRole::face_detection:
        return "face_detection";
    case ModelRole::facial_landmarks:
        return "facial_landmarks";
    case ModelRole::head_pose:
        return "head_pose";
    case ModelRole::facial_landmarks_35:
        return "facial_landmarks_35";
    }

    return "unknown";
}

}  // namespace npuvt::models
