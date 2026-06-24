#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "npu_vtube/models/model_catalog.hpp"

namespace npuvt::models {

struct ModelResolverOptions {
    std::filesystem::path models_root;
};

class ModelResolver {
public:
    explicit ModelResolver(ModelResolverOptions options = {});

    [[nodiscard]] const ModelResolverOptions& options() const noexcept;
    void set_options(ModelResolverOptions options);
    [[nodiscard]] ResolvedModel resolve(const ModelDefinition& definition) const;
    [[nodiscard]] std::vector<ResolvedModel> resolve_defaults(const ModelCatalog& catalog) const;

private:
    [[nodiscard]] static std::filesystem::path find_model_file(
        const std::filesystem::path& root,
        const std::string& model_id,
        const std::vector<std::string>& extensions);

    ModelResolverOptions options_ {};
};

}  // namespace npuvt::models
