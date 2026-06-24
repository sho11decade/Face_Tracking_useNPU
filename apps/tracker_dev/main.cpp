#include <iostream>
#include <vector>

#include "npu_vtube/capture/capture_service.hpp"
#include "npu_vtube/core/application_info.hpp"
#include "npu_vtube/inference/inference_service.hpp"
#include "npu_vtube/mapping/parameter_mapper.hpp"
#include "npu_vtube/platform/windows/environment.hpp"
#include "npu_vtube/render/render_backend.hpp"
#include "npu_vtube/tracking/tracking_service.hpp"
#include "npu_vtube/ui/debug_ui.hpp"

int main() {
    const auto app_info = npuvt::core::make_application_info();

    npuvt::capture::CaptureService capture_service;
    npuvt::inference::InferenceService inference_service;
    npuvt::tracking::TrackingService tracking_service;
    npuvt::mapping::ParameterMapper parameter_mapper;
    npuvt::render::RenderBackend render_backend;
    npuvt::ui::DebugUi debug_ui;

    std::vector<std::string> available_devices {"CPU"};
    if (npuvt::platform::windows::is_npu_driver_expected()) {
        available_devices.insert(available_devices.begin(), "NPU");
    }

    const auto device = inference_service.select_device(available_devices);
    const auto observation = inference_service.make_placeholder_observation();
    const auto tracking_state = tracking_service.update(observation);
    const auto mapped = parameter_mapper.map(tracking_state);

    std::cout << app_info.name << " v" << app_info.version << '\n';
    std::cout << "Capture stage: " << capture_service.describe() << '\n';
    std::cout << "Inference device: " << device << '\n';
    std::cout << "Render backend: " << render_backend.name() << '\n';
    std::cout << "UI backend: " << debug_ui.name() << '\n';
    std::cout << "Mapped yaw/pitch/roll: " << mapped.angle_yaw << ", " << mapped.angle_pitch << ", "
              << mapped.angle_roll << '\n';

    return 0;
}
