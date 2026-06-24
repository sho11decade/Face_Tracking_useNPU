#pragma once

#include "npu_vtube/core/types.hpp"

namespace npuvt::mapping {

class ParameterMapper {
public:
    [[nodiscard]] npuvt::core::MappedParameters map(const npuvt::core::TrackingState& state) const;
};

}  // namespace npuvt::mapping
