#pragma once

#include "npu_vtube/core/types.hpp"

namespace npuvt::tracking {

class TrackingService {
public:
    [[nodiscard]] npuvt::core::TrackingState update(const npuvt::core::FaceObservation& observation) const;
};

}  // namespace npuvt::tracking
