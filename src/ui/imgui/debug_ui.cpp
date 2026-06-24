#include "npu_vtube/ui/debug_ui.hpp"

namespace npuvt::ui {

DebugUi::DebugUi() = default;

std::string_view DebugUi::name() const noexcept {
    return "Dear ImGui";
}

DebugUiState DebugUi::state() const {
    return state_;
}

void DebugUi::set_state(DebugUiState state) {
    state_ = state;
}

bool DebugUi::is_ready() const noexcept {
    return ready_;
}

void DebugUi::submit(const npuvt::render::RenderFrameInfo& frame_info) {
    (void)frame_info;
    ready_ = true;
}

}  // namespace npuvt::ui
