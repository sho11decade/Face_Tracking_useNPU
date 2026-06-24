#include "npu_vtube/capture/capture_service.hpp"

#include <chrono>

namespace npuvt::capture {

std::string_view CaptureService::describe() const noexcept {
    return "Media Foundation / OpenCV capture stub";
}

npuvt::core::FramePacket CaptureService::make_placeholder_frame() const {
    return {
        .frame_id = 0,
        .timestamp = std::chrono::steady_clock::now(),
        .width = 1280,
        .height = 720,
        .format = npuvt::core::PixelFormat::bgr8,
        .image = {},
    };
}

}  // namespace npuvt::capture
