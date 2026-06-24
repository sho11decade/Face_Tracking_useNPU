#include "npu_vtube/inference/inference_service.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <utility>

namespace npuvt::inference {

InferenceService::InferenceService() = default;

InferenceService::InferenceService(InferenceOptions options) : options_(std::move(options)) {
}

void InferenceService::set_options(InferenceOptions options) {
    options_ = std::move(options);
}

InferenceOptions InferenceService::options() const {
    return options_;
}

std::string InferenceService::select_device(const std::vector<std::string>& available_devices) const {
    if (!options_.device_hint.empty()) {
        const auto it = std::find(available_devices.begin(), available_devices.end(), options_.device_hint);
        if (it != available_devices.end()) {
            return *it;
        }
    }

    const auto& preferred_order = options_.preferred_devices.empty()
                                      ? std::vector<std::string> {"NPU", "GPU", "CPU"}
                                      : options_.preferred_devices;

    for (const auto& preferred : preferred_order) {
        const auto it = std::find(available_devices.begin(), available_devices.end(), preferred);
        if (it != available_devices.end()) {
            return *it;
        }
    }

    return available_devices.empty() ? "UNAVAILABLE" : available_devices.front();
}

const npuvt::models::ModelCatalog& InferenceService::model_catalog() const noexcept {
    return model_catalog_;
}

npuvt::models::ModelResolver InferenceService::model_resolver() const {
    npuvt::models::ModelResolver resolver;
    resolver.set_options({.models_root = options_.models_root});
    return resolver;
}

std::optional<npuvt::models::ResolvedModel> InferenceService::resolve_model(npuvt::models::ModelRole role) const {
    const auto* definition = model_catalog_.find(role);
    if (definition == nullptr) {
        return std::nullopt;
    }

    const auto resolver = model_resolver();
    const auto resolved = resolver.resolve(*definition);
    return resolved;
}

ModelLoadSummary InferenceService::load_model_summary() const {
    ModelLoadSummary summary {};

    if (const auto face = resolve_model(npuvt::models::ModelRole::face_detection); face.has_value()) {
        summary.face_detection = *face;
        summary.face_detection_ready = face->xml_exists || face->onnx_exists;
    }

    if (const auto landmarks = resolve_model(npuvt::models::ModelRole::facial_landmarks); landmarks.has_value()) {
        summary.facial_landmarks = *landmarks;
        summary.facial_landmarks_ready = landmarks->xml_exists || landmarks->onnx_exists;
    }

    if (const auto head_pose = resolve_model(npuvt::models::ModelRole::head_pose); head_pose.has_value()) {
        summary.head_pose = *head_pose;
        summary.head_pose_ready = head_pose->xml_exists || head_pose->onnx_exists;
    }

    return summary;
}

npuvt::core::FaceObservation InferenceService::analyze(const npuvt::core::FramePacket& frame) const {
    if (frame.width <= 0 || frame.height <= 0 || frame.image.bytes.empty()) {
        return make_placeholder_observation();
    }

    const float width = static_cast<float>(frame.width);
    const float height = static_cast<float>(frame.height);
    const float face_width = width * 0.33F;
    const float face_height = height * 0.45F;
    const float face_x = (width * 0.5F) - (face_width * 0.5F);
    const float face_y = (height * 0.40F) - (face_height * 0.5F);

    return {
        .frame_id = frame.frame_id,
        .timestamp = frame.timestamp,
        .frame_width = frame.width,
        .frame_height = frame.height,
        .detected = true,
        .face_bbox = {.x = face_x, .y = face_y, .width = face_width, .height = face_height},
        .landmarks = {},
        .yaw = 0.0F,
        .pitch = 0.0F,
        .roll = 0.0F,
        .confidence = 0.75F,
    };
}

npuvt::core::FaceObservation InferenceService::make_placeholder_observation() const {
    return {
        .frame_id = 0,
        .timestamp = std::chrono::steady_clock::now(),
        .frame_width = 1280,
        .frame_height = 720,
        .detected = true,
        .face_bbox = {.x = 430.0F, .y = 180.0F, .width = 420.0F, .height = 420.0F},
        .landmarks = {},
        .yaw = 0.0F,
        .pitch = 0.0F,
        .roll = 0.0F,
        .confidence = 1.0F,
    };
}

}  // namespace npuvt::inference
