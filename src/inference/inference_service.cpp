#include "npu_vtube/inference/inference_service.hpp"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#if defined(NPUVT_ENABLE_OPENVINO)
#include <openvino/openvino.hpp>
#endif

namespace npuvt::inference {

namespace {

npuvt::core::FaceObservation make_fallback_observation(const npuvt::core::FramePacket& frame, bool detected, float confidence) {
    const float width = frame.width > 0 ? static_cast<float>(frame.width) : 1280.0F;
    const float height = frame.height > 0 ? static_cast<float>(frame.height) : 720.0F;
    const float face_width = width * 0.33F;
    const float face_height = height * 0.45F;
    const float face_x = (width * 0.5F) - (face_width * 0.5F);
    const float face_y = (height * 0.40F) - (face_height * 0.5F);

    return {
        .frame_id = frame.frame_id,
        .timestamp = frame.timestamp,
        .frame_width = frame.width > 0 ? frame.width : 1280,
        .frame_height = frame.height > 0 ? frame.height : 720,
        .detected = detected,
        .face_bbox = {.x = face_x, .y = face_y, .width = face_width, .height = face_height},
        .landmarks = {},
        .yaw = 0.0F,
        .pitch = 0.0F,
        .roll = 0.0F,
        .confidence = confidence,
    };
}

std::filesystem::path choose_face_detector_path(
    const InferenceOptions& options,
    const std::optional<npuvt::models::ResolvedModel>& resolved_model) {
    if (!options.model_path.empty()) {
        return options.model_path;
    }

    if (!resolved_model.has_value()) {
        return {};
    }

    if (options.prefer_onnx && resolved_model->onnx_exists) {
        return resolved_model->onnx_path;
    }

    if (resolved_model->xml_exists && resolved_model->bin_exists) {
        return resolved_model->xml_path;
    }

    if (resolved_model->onnx_exists) {
        return resolved_model->onnx_path;
    }

    return {};
}

#if defined(NPUVT_ENABLE_OPENVINO)

void read_source_bgr(const npuvt::core::FramePacket& frame, int x, int y, std::uint8_t* b, std::uint8_t* g, std::uint8_t* r) {
    const std::size_t pixel_index = static_cast<std::size_t>((y * frame.width) + x);

    switch (frame.format) {
    case npuvt::core::PixelFormat::bgra8: {
        const std::size_t base = pixel_index * 4U;
        *b = std::to_integer<std::uint8_t>(frame.image.bytes[base + 0]);
        *g = std::to_integer<std::uint8_t>(frame.image.bytes[base + 1]);
        *r = std::to_integer<std::uint8_t>(frame.image.bytes[base + 2]);
        return;
    }
    case npuvt::core::PixelFormat::bgr8: {
        const std::size_t base = pixel_index * 3U;
        *b = std::to_integer<std::uint8_t>(frame.image.bytes[base + 0]);
        *g = std::to_integer<std::uint8_t>(frame.image.bytes[base + 1]);
        *r = std::to_integer<std::uint8_t>(frame.image.bytes[base + 2]);
        return;
    }
    case npuvt::core::PixelFormat::rgb8: {
        const std::size_t base = pixel_index * 3U;
        *r = std::to_integer<std::uint8_t>(frame.image.bytes[base + 0]);
        *g = std::to_integer<std::uint8_t>(frame.image.bytes[base + 1]);
        *b = std::to_integer<std::uint8_t>(frame.image.bytes[base + 2]);
        return;
    }
    default:
        throw std::runtime_error("Unsupported frame pixel format for OpenVINO preprocessing.");
    }
}

ov::Tensor make_input_tensor(
    const npuvt::core::FramePacket& frame,
    const ov::Shape& input_shape,
    const ov::element::Type& element_type) {
    if (input_shape.size() != 4 || input_shape[0] != 1 || input_shape[1] != 3) {
        throw std::runtime_error("Face detector input must be static NCHW with 3 channels.");
    }

    const std::size_t input_height = input_shape[2];
    const std::size_t input_width = input_shape[3];
    ov::Tensor tensor(element_type, input_shape);

    auto fill_pixel = [&](auto* dst) {
        const std::size_t plane_size = input_width * input_height;
        for (std::size_t y = 0; y < input_height; ++y) {
            const int source_y = static_cast<int>((y * static_cast<std::size_t>(frame.height)) / input_height);
            for (std::size_t x = 0; x < input_width; ++x) {
                const int source_x = static_cast<int>((x * static_cast<std::size_t>(frame.width)) / input_width);
                std::uint8_t b = 0;
                std::uint8_t g = 0;
                std::uint8_t r = 0;
                read_source_bgr(frame, source_x, source_y, &b, &g, &r);
                const std::size_t offset = (y * input_width) + x;
                using ValueType = std::remove_pointer_t<decltype(dst)>;
                dst[(0 * plane_size) + offset] = static_cast<ValueType>(b);
                dst[(1 * plane_size) + offset] = static_cast<ValueType>(g);
                dst[(2 * plane_size) + offset] = static_cast<ValueType>(r);
            }
        }
    };

    if (element_type == ov::element::u8) {
        fill_pixel(tensor.data<std::uint8_t>());
        return tensor;
    }

    if (element_type == ov::element::f32) {
        fill_pixel(tensor.data<float>());
        return tensor;
    }

    throw std::runtime_error("Unsupported OpenVINO input element type.");
}

npuvt::core::FaceObservation parse_face_detection_output(
    const npuvt::core::FramePacket& frame,
    const ov::Tensor& output_tensor) {
    if (output_tensor.get_element_type() != ov::element::f32) {
        throw std::runtime_error("Face detector output element type must be f32.");
    }

    const auto& shape = output_tensor.get_shape();
    if (shape.empty() || shape.back() != 7) {
        throw std::runtime_error("Unexpected face detector output shape.");
    }

    const auto* data = output_tensor.data<const float>();
    const std::size_t detection_count = output_tensor.get_size() / 7U;

    float best_confidence = 0.0F;
    npuvt::core::Rect2f best_bbox {};

    for (std::size_t i = 0; i < detection_count; ++i) {
        const float image_id = data[(i * 7U) + 0U];
        const float confidence = data[(i * 7U) + 2U];
        if (image_id < 0.0F || confidence < best_confidence) {
            continue;
        }

        const float x_min = std::clamp(data[(i * 7U) + 3U], 0.0F, 1.0F);
        const float y_min = std::clamp(data[(i * 7U) + 4U], 0.0F, 1.0F);
        const float x_max = std::clamp(data[(i * 7U) + 5U], 0.0F, 1.0F);
        const float y_max = std::clamp(data[(i * 7U) + 6U], 0.0F, 1.0F);

        best_confidence = confidence;
        best_bbox = {
            .x = x_min * static_cast<float>(frame.width),
            .y = y_min * static_cast<float>(frame.height),
            .width = (x_max - x_min) * static_cast<float>(frame.width),
            .height = (y_max - y_min) * static_cast<float>(frame.height),
        };
    }

    if (best_confidence <= 0.0F) {
        return make_fallback_observation(frame, false, 0.0F);
    }

    return {
        .frame_id = frame.frame_id,
        .timestamp = frame.timestamp,
        .frame_width = frame.width,
        .frame_height = frame.height,
        .detected = true,
        .face_bbox = best_bbox,
        .landmarks = {},
        .yaw = 0.0F,
        .pitch = 0.0F,
        .roll = 0.0F,
        .confidence = best_confidence,
    };
}

#endif

}  // namespace

struct InferenceService::Impl {
    InferenceRuntimeStatus status {};

#if defined(NPUVT_ENABLE_OPENVINO)
    bool core_initialized {false};
    ov::Core core;
    ov::CompiledModel face_detector_model;
    ov::Shape face_detector_input_shape;
    ov::element::Type face_detector_input_type {ov::element::undefined};
#endif
};

InferenceService::InferenceService() : impl_(std::make_unique<Impl>()) {
    impl_->status.openvino_compiled =
#if defined(NPUVT_ENABLE_OPENVINO)
        true;
#else
        false;
#endif
}

InferenceService::~InferenceService() = default;

InferenceService::InferenceService(InferenceOptions options) : InferenceService() {
    options_ = std::move(options);
}

InferenceService::InferenceService(InferenceService&& other) noexcept = default;

InferenceService& InferenceService::operator=(InferenceService&& other) noexcept = default;

void InferenceService::set_options(InferenceOptions options) {
    options_ = std::move(options);
    impl_->status.runtime_ready = false;
    impl_->status.face_detector_compiled = false;
    impl_->status.face_detector_path.clear();
    impl_->status.last_error.clear();
}

InferenceOptions InferenceService::options() const {
    return options_;
}

std::vector<std::string> InferenceService::query_available_devices() {
#if defined(NPUVT_ENABLE_OPENVINO)
    try {
        if (!impl_->core_initialized) {
            impl_->core = ov::Core {};
            impl_->core_initialized = true;
        }
        return impl_->core.get_available_devices();
    } catch (const std::exception& error) {
        impl_->status.last_error = error.what();
        return {};
    }
#else
    return {};
#endif
}

bool InferenceService::initialize_runtime(std::string* error_message) {
    auto set_error = [&](const std::string& message) {
        impl_->status.last_error = message;
        if (error_message != nullptr) {
            *error_message = message;
        }
    };

#if !defined(NPUVT_ENABLE_OPENVINO)
    set_error("OpenVINO support is disabled in this build. Configure with -DNPUVT_ENABLE_OPENVINO=ON.");
    return false;
#else
    try {
        auto available_devices = query_available_devices();
        if (available_devices.empty()) {
            set_error(impl_->status.last_error.empty() ? "No OpenVINO devices available." : impl_->status.last_error);
            return false;
        }

        const auto selected_device = select_device(available_devices);
        const auto resolved_face_model = resolve_model(npuvt::models::ModelRole::face_detection);
        const auto face_model_path = choose_face_detector_path(options_, resolved_face_model);
        if (face_model_path.empty()) {
            set_error("No face detection model file found. Place ONNX or IR files under models/.");
            return false;
        }

        auto model = impl_->core.read_model(face_model_path.string());
        const auto input = model->input();
        const auto input_shape = input.get_shape();
        if (input_shape.size() != 4 || input_shape[0] != 1 || input_shape[1] != 3) {
            set_error("Face detection model must use static NCHW input with 3 channels.");
            return false;
        }

        impl_->face_detector_input_shape = input_shape;
        impl_->face_detector_input_type = input.get_element_type();
        impl_->face_detector_model = impl_->core.compile_model(
            model,
            selected_device,
            ov::hint::performance_mode(ov::hint::PerformanceMode::LATENCY));

        impl_->status.runtime_ready = true;
        impl_->status.face_detector_compiled = true;
        impl_->status.selected_device = selected_device;
        impl_->status.face_detector_path = face_model_path.string();
        impl_->status.last_error.clear();
        return true;
    } catch (const std::exception& error) {
        set_error(error.what());
        return false;
    }
#endif
}

InferenceRuntimeStatus InferenceService::runtime_status() const {
    return impl_->status;
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
    return resolver.resolve(*definition);
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

npuvt::core::FaceObservation InferenceService::analyze(const npuvt::core::FramePacket& frame) {
    if (frame.width <= 0 || frame.height <= 0 || frame.image.bytes.empty()) {
        return make_placeholder_observation();
    }

#if defined(NPUVT_ENABLE_OPENVINO)
    if (!impl_->status.face_detector_compiled) {
        initialize_runtime(nullptr);
    }

    if (impl_->status.face_detector_compiled) {
        try {
            auto infer_request = impl_->face_detector_model.create_infer_request();
            auto input_tensor = make_input_tensor(frame, impl_->face_detector_input_shape, impl_->face_detector_input_type);
            infer_request.set_input_tensor(input_tensor);
            infer_request.infer();
            return parse_face_detection_output(frame, infer_request.get_output_tensor(0));
        } catch (const std::exception& error) {
            impl_->status.last_error = error.what();
        }
    }
#endif

    return make_fallback_observation(frame, true, 0.75F);
}

npuvt::core::FaceObservation InferenceService::make_placeholder_observation() const {
    npuvt::core::FramePacket placeholder_frame {};
    placeholder_frame.width = 1280;
    placeholder_frame.height = 720;
    placeholder_frame.timestamp = std::chrono::steady_clock::now();
    return make_fallback_observation(placeholder_frame, true, 1.0F);
}

}  // namespace npuvt::inference
