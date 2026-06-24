#pragma once

#include "npu_vtube/render/render_backend.hpp"

#include <string_view>

namespace npuvt::ui {

struct DebugUiState {
    bool show_landmarks {true};
    bool show_head_pose {true};
    bool show_performance {true};
};

class DebugUi {
public:
    DebugUi();

    [[nodiscard]] std::string_view name() const noexcept;
    [[nodiscard]] DebugUiState state() const;
    void set_state(DebugUiState state);
    [[nodiscard]] bool is_ready() const noexcept;
    void submit(const npuvt::render::RenderFrameInfo& frame_info);

private:
    DebugUiState state_ {};
    bool ready_ {false};
};

}  // namespace npuvt::ui
