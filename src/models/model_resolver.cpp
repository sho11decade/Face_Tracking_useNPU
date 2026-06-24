#include "npu_vtube/models/model_resolver.hpp"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>
#include <utility>

namespace npuvt::models {

namespace {

std::filesystem::path append_if_exists(const std::filesystem::path& candidate, const std::string& extension) {
    auto path = candidate;
    path.replace_extension(extension);
    return path;
}

}  // namespace

ModelResolver::ModelResolver(ModelResolverOptions options) : options_(std::move(options)) {
}

const ModelResolverOptions& ModelResolver::options() const noexcept {
    return options_;
}

void ModelResolver::set_options(ModelResolverOptions options) {
    options_ = std::move(options);
}

std::filesystem::path ModelResolver::find_model_file(
    const std::filesystem::path& root,
    const std::string& model_id,
    const std::vector<std::string>& extensions) {
    if (root.empty() || model_id.empty()) {
        return {};
    }

    const auto direct_base = root / model_id;
    for (const auto& ext : extensions) {
        const auto direct = append_if_exists(direct_base, ext);
        if (std::filesystem::exists(direct)) {
            return direct;
        }

        const auto nested = append_if_exists(root / model_id / model_id, ext);
        if (std::filesystem::exists(nested)) {
            return nested;
        }
    }

    return {};
}

ResolvedModel ModelResolver::resolve(const ModelDefinition& definition) const {
    ResolvedModel resolved {};
    resolved.definition = definition;
    resolved.root = options_.models_root;
    resolved.xml_path = find_model_file(options_.models_root, definition.model_id, {".xml", ".XML"});
    resolved.bin_path = find_model_file(options_.models_root, definition.model_id, {".bin", ".BIN"});
    resolved.onnx_path = find_model_file(options_.models_root, definition.model_id, {".onnx", ".ONNX"});
    resolved.xml_exists = !resolved.xml_path.empty() && std::filesystem::exists(resolved.xml_path);
    resolved.bin_exists = !resolved.bin_path.empty() && std::filesystem::exists(resolved.bin_path);
    resolved.onnx_exists = !resolved.onnx_path.empty() && std::filesystem::exists(resolved.onnx_path);
    return resolved;
}

std::vector<ResolvedModel> ModelResolver::resolve_defaults(const ModelCatalog& catalog) const {
    std::vector<ResolvedModel> resolved_models;
    const auto& default_models = catalog.defaults();
    resolved_models.reserve(default_models.size());
    for (const auto& model : default_models) {
        resolved_models.push_back(resolve(model));
    }
    return resolved_models;
}

}  // namespace npuvt::models
