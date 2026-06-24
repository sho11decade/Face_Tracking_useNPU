#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace npuvt::core {

enum class PixelFormat {
    unknown,
    bgr8,
    rgb8,
    bgra8,
    nv12,
};

struct Vec2f {
    float x {};
    float y {};
};

struct Rect2f {
    float x {};
    float y {};
    float width {};
    float height {};
};

struct ImageBuffer {
    std::vector<std::byte> bytes;
};

struct FramePacket {
    std::uint64_t frame_id {};
    std::chrono::steady_clock::time_point timestamp {};
    int width {};
    int height {};
    PixelFormat format {PixelFormat::unknown};
    ImageBuffer image;
};

struct FaceObservation {
    std::uint64_t frame_id {};
    std::chrono::steady_clock::time_point timestamp {};
    bool detected {};
    Rect2f face_bbox {};
    std::vector<Vec2f> landmarks;
    float yaw {};
    float pitch {};
    float roll {};
    float confidence {};
};

struct TrackingState {
    std::uint64_t frame_id {};
    std::chrono::steady_clock::time_point timestamp {};
    bool valid {};
    Vec2f face_center {};
    float face_scale {};
    float yaw {};
    float pitch {};
    float roll {};
};

struct MappedParameters {
    float head_x {};
    float head_y {};
    float head_z {};
    float angle_yaw {};
    float angle_pitch {};
    float angle_roll {};
};

struct PerfSnapshot {
    float capture_ms {};
    float preprocess_ms {};
    float inference_ms {};
    float filter_ms {};
    float render_ms {};
    float end_to_end_ms {};
    float fps {};
    std::string device_name;
};

}  // namespace npuvt::core
