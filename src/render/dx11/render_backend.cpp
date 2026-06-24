#include "npu_vtube/render/render_backend.hpp"

namespace npuvt::render {

RenderBackend::RenderBackend() = default;

std::string_view RenderBackend::name() const noexcept {
    return "DirectX11";
}

bool RenderBackend::is_ready() const noexcept {
    return ready_;
}

RenderFrameInfo RenderBackend::last_frame_info() const {
    return last_frame_info_;
}

void RenderBackend::submit(RenderFrameInfo frame_info) {
    last_frame_info_ = frame_info;
    ready_ = true;
}

}  // namespace npuvt::render
