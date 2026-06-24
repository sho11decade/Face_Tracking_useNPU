#pragma once

#include <string>
#include <vector>

#include "npu_vtube/core/types.hpp"

namespace npuvt::inference {

class InferenceService {
public:
    [[nodiscard]] std::string select_device(const std::vector<std::string>& available_devices) const;
    [[nodiscard]] npuvt::core::FaceObservation make_placeholder_observation() const;
};

}  // namespace npuvt::inference
