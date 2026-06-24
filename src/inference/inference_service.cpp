#include "npu_vtube/inference/inference_service.hpp"

#include <algorithm>
#include <array>
#include <chrono>

namespace npuvt::inference {

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

npuvt::core::FaceObservation InferenceService::make_placeholder_observation() const {
    return {
        .frame_id = 0,
        .timestamp = std::chrono::steady_clock::now(),
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
