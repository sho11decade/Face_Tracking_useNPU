#pragma once

#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "npu_vtube/core/types.hpp"
#include "npu_vtube/models/model_catalog.hpp"
#include "npu_vtube/models/model_resolver.hpp"

namespace npuvt::inference {

struct InferenceOptions {
    std::vector<std::string> preferred_devices {"NPU", "GPU", "CPU"};
    std::string model_path;
    std::string device_hint;
    std::string models_root {"models"};
    bool prefer_onnx {true};
};

struct ModelLoadSummary {
    npuvt::models::ResolvedModel face_detection;
    npuvt::models::ResolvedModel facial_landmarks;
    npuvt::models::ResolvedModel head_pose;
    bool face_detection_ready {false};
    bool facial_landmarks_ready {false};
    bool head_pose_ready {false};
};

struct InferenceRuntimeStatus {
    bool openvino_compiled {false};
    bool runtime_ready {false};
    bool face_detector_compiled {false};
    std::string selected_device;
    std::string face_detector_path;
    std::string last_error;
};

class InferenceService {
public:
    InferenceService();
    ~InferenceService();

    explicit InferenceService(InferenceOptions options);

    InferenceService(InferenceService&& other) noexcept;
    InferenceService& operator=(InferenceService&& other) noexcept;

    InferenceService(const InferenceService&) = delete;
    InferenceService& operator=(const InferenceService&) = delete;

    void set_options(InferenceOptions options);
    [[nodiscard]] InferenceOptions options() const;

    [[nodiscard]] std::vector<std::string> query_available_devices();
    [[nodiscard]] bool initialize_runtime(std::string* error_message = nullptr);
    [[nodiscard]] InferenceRuntimeStatus runtime_status() const;

    [[nodiscard]] std::string select_device(const std::vector<std::string>& available_devices) const;
    [[nodiscard]] const npuvt::models::ModelCatalog& model_catalog() const noexcept;
    [[nodiscard]] npuvt::models::ModelResolver model_resolver() const;
    [[nodiscard]] std::optional<npuvt::models::ResolvedModel> resolve_model(npuvt::models::ModelRole role) const;
    [[nodiscard]] ModelLoadSummary load_model_summary() const;
    [[nodiscard]] npuvt::core::FaceObservation analyze(const npuvt::core::FramePacket& frame);
    [[nodiscard]] npuvt::core::FaceObservation make_placeholder_observation() const;

private:
    struct Impl;

    InferenceOptions options_ {};
    npuvt::models::ModelCatalog model_catalog_ {};
    std::unique_ptr<Impl> impl_;
};

}  // namespace npuvt::inference
