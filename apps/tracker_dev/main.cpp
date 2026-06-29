#include <iostream>
#include <string>
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

    const auto model_summary = inference_service.load_model_summary();
    auto available_devices = inference_service.query_available_devices();
    if (available_devices.empty()) {
        available_devices = {"CPU"};
        if (npuvt::platform::windows::is_npu_driver_expected()) {
            available_devices.insert(available_devices.begin(), "NPU");
        }
    }

    const auto device = inference_service.select_device(available_devices);
    std::string runtime_error;
    const auto runtime_initialized = inference_service.initialize_runtime(&runtime_error);
    const auto runtime_status = inference_service.runtime_status();
    const auto placeholder_observation = inference_service.make_placeholder_observation();
    const auto tracking_state = tracking_service.update(placeholder_observation);
    const auto mapped = parameter_mapper.map(tracking_state);
    const auto capture_devices = capture_service.list_devices();

    std::cout << app_info.name << " v" << app_info.version << '\n';
    std::cout << "Capture stage: " << capture_service.describe() << '\n';
    std::cout << "Detected cameras: " << capture_devices.size() << '\n';
    for (std::size_t i = 0; i < capture_devices.size(); ++i) {
        std::cout << "  [" << i << "] " << capture_devices[i].name << '\n';
    }
    std::cout << "Inference device: " << device << '\n';
    std::cout << "OpenVINO compiled in: " << (runtime_status.openvino_compiled ? "yes" : "no") << '\n';
    std::cout << "OpenVINO runtime ready: " << (runtime_initialized ? "yes" : "no") << '\n';
    if (!runtime_initialized && !runtime_error.empty()) {
        std::cout << "OpenVINO init error: " << runtime_error << '\n';
    }
    std::cout << "Face detector ready: " << (model_summary.face_detection_ready ? "yes" : "no") << '\n';
    std::cout << "Render backend: " << render_backend.name() << '\n';
    std::cout << "UI backend: " << debug_ui.name() << '\n';
    std::cout << "Mapped yaw/pitch/roll: " << mapped.angle_yaw << ", " << mapped.angle_pitch << ", "
              << mapped.angle_roll << '\n';

    render_backend.submit({.frame = capture_service.make_placeholder_frame(), .tracking_state = tracking_state, .mapped_parameters = mapped});
    debug_ui.submit(render_backend.last_frame_info());

    if (!capture_devices.empty()) {
        std::string capture_error;
        npuvt::capture::CaptureOptions capture_options;
        capture_options.device_index = 0;

        if (capture_service.initialize(capture_options, &capture_error)) {
            const auto frame = capture_service.grab_frame(&capture_error);
            std::cout << "Captured frame: " << frame.width << "x" << frame.height << ", bytes="
                      << frame.image.bytes.size() << '\n';
            const auto live_observation = inference_service.analyze(frame);
            const auto live_tracking_state = tracking_service.update(live_observation);
            const auto live_mapped = parameter_mapper.map(live_tracking_state);
            std::cout << "Live head position: " << live_mapped.head_x << ", " << live_mapped.head_y << ", "
                      << live_mapped.head_z << '\n';
            render_backend.submit({.frame = frame, .tracking_state = live_tracking_state, .mapped_parameters = live_mapped});
            debug_ui.submit(render_backend.last_frame_info());
            capture_service.shutdown();
        } else {
            std::cout << "Capture initialization failed: " << capture_error << '\n';
        }
    }

    return 0;
}
