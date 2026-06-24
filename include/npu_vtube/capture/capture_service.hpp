#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <string_view>

#include "npu_vtube/core/types.hpp"

namespace npuvt::capture {

struct CaptureDeviceInfo {
    std::string name;
    std::string symbolic_link;
};

struct CaptureOptions {
    std::uint32_t device_index {0};
    std::uint32_t width {1280};
    std::uint32_t height {720};
    npuvt::core::PixelFormat preferred_format {npuvt::core::PixelFormat::bgra8};
};

class CaptureService {
public:
    CaptureService();
    ~CaptureService();

    CaptureService(CaptureService&& other) noexcept;
    CaptureService& operator=(CaptureService&& other) noexcept;

    CaptureService(const CaptureService&) = delete;
    CaptureService& operator=(const CaptureService&) = delete;

    [[nodiscard]] std::string_view describe() const noexcept;
    [[nodiscard]] std::vector<CaptureDeviceInfo> list_devices() const;
    [[nodiscard]] bool initialize(const CaptureOptions& options, std::string* error_message = nullptr);
    void shutdown() noexcept;
    [[nodiscard]] bool is_initialized() const noexcept;
    [[nodiscard]] npuvt::core::FramePacket grab_frame(std::string* error_message = nullptr);
    [[nodiscard]] npuvt::core::FramePacket make_placeholder_frame() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace npuvt::capture
