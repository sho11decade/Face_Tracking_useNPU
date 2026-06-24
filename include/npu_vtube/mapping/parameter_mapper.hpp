#pragma once

#include "npu_vtube/core/types.hpp"

namespace npuvt::mapping {

struct MappingOptions {
    float max_center_offset_ratio {1.0F};
    float scale_reference_ratio {0.35F};
};

class ParameterMapper {
public:
    ParameterMapper();
    explicit ParameterMapper(MappingOptions options);

    void set_options(MappingOptions options);
    [[nodiscard]] MappingOptions options() const;
    [[nodiscard]] npuvt::core::MappedParameters map(const npuvt::core::TrackingState& state) const;

private:
    MappingOptions options_ {};
};

}  // namespace npuvt::mapping
