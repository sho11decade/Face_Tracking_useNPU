#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace npuvt::models {

enum class ModelRole {
    face_detection,
    facial_landmarks,
    head_pose,
    facial_landmarks_35,
};

struct ModelDefinition {
    ModelRole role {};
    std::string model_id;
    std::string display_name;
    std::string description;
    std::uint32_t input_width {};
    std::uint32_t input_height {};
    std::uint32_t input_channels {3};
    std::string input_color_order {"BGR"};
    bool recommended_for_npu {true};
};

struct ResolvedModel {
    ModelDefinition definition;
    std::filesystem::path root;
    std::filesystem::path xml_path;
    std::filesystem::path bin_path;
    std::filesystem::path onnx_path;
    bool xml_exists {false};
    bool bin_exists {false};
    bool onnx_exists {false};
};

class ModelCatalog {
public:
    [[nodiscard]] const std::vector<ModelDefinition>& defaults() const;
    [[nodiscard]] const ModelDefinition* find(ModelRole role) const;

private:
    mutable std::vector<ModelDefinition> defaults_;
};

std::string_view to_string(ModelRole role) noexcept;

}  // namespace npuvt::models
