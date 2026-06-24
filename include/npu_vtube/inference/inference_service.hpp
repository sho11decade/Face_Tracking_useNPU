#pragma once

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

class InferenceService {
public:
    InferenceService();
    explicit InferenceService(InferenceOptions options);

    void set_options(InferenceOptions options);
    [[nodiscard]] InferenceOptions options() const;

    [[nodiscard]] std::string select_device(const std::vector<std::string>& available_devices) const;
    [[nodiscard]] const npuvt::models::ModelCatalog& model_catalog() const noexcept;
    [[nodiscard]] npuvt::models::ModelResolver model_resolver() const;
    [[nodiscard]] std::optional<npuvt::models::ResolvedModel> resolve_model(npuvt::models::ModelRole role) const;
    [[nodiscard]] ModelLoadSummary load_model_summary() const;
    [[nodiscard]] npuvt::core::FaceObservation analyze(const npuvt::core::FramePacket& frame) const;
    [[nodiscard]] npuvt::core::FaceObservation make_placeholder_observation() const;

private:
    InferenceOptions options_ {};
    npuvt::models::ModelCatalog model_catalog_ {};
};

}  // namespace npuvt::inference
