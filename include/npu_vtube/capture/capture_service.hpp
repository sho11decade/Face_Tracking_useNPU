#pragma once

#include <string_view>

#include "npu_vtube/core/types.hpp"

namespace npuvt::capture {

class CaptureService {
public:
    [[nodiscard]] std::string_view describe() const noexcept;
    [[nodiscard]] npuvt::core::FramePacket make_placeholder_frame() const;
};

}  // namespace npuvt::capture
