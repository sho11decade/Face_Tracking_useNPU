#pragma once

#include <string>
#include <vector>

#include "npu_vtube/core/types.hpp"

namespace npuvt::inference {

struct InferenceOptions {
    std::vector<std::string> preferred_devices {"NPU", "GPU", "CPU"};
    std::string model_path;
    std::string device_hint;
};

class InferenceService {
public:
    InferenceService();
    explicit InferenceService(InferenceOptions options);

    void set_options(InferenceOptions options);
    [[nodiscard]] InferenceOptions options() const;

    [[nodiscard]] std::string select_device(const std::vector<std::string>& available_devices) const;
    [[nodiscard]] npuvt::core::FaceObservation analyze(const npuvt::core::FramePacket& frame) const;
    [[nodiscard]] npuvt::core::FaceObservation make_placeholder_observation() const;

private:
    InferenceOptions options_ {};
};

}  // namespace npuvt::inference
