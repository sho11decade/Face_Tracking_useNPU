#pragma once

#include "npu_vtube/core/types.hpp"

#include <string_view>

namespace npuvt::render {

struct RenderFrameInfo {
    npuvt::core::FramePacket frame;
    npuvt::core::TrackingState tracking_state;
    npuvt::core::MappedParameters mapped_parameters;
};

class RenderBackend {
public:
    RenderBackend();

    [[nodiscard]] std::string_view name() const noexcept;
    [[nodiscard]] bool is_ready() const noexcept;
    [[nodiscard]] RenderFrameInfo last_frame_info() const;
    void submit(RenderFrameInfo frame_info);

private:
    bool ready_ {false};
    RenderFrameInfo last_frame_info_ {};
};

}  // namespace npuvt::render
