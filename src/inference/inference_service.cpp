#include "npu_vtube/inference/inference_service.hpp"

#include <algorithm>
#include <array>
#include <chrono>

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
    static constexpr std::array preferred_order {"NPU", "GPU", "CPU"};

    for (const auto* preferred : preferred_order) {
        const auto it = std::find(available_devices.begin(), available_devices.end(), preferred);
        if (it != available_devices.end()) {
            return *it;
        }
    }

    return available_devices.empty() ? "UNAVAILABLE" : available_devices.front();
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
