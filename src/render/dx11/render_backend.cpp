#include "npu_vtube/render/render_backend.hpp"

namespace npuvt::render {

std::string_view RenderBackend::name() const noexcept {
    return "DirectX11 stub";
}

}  // namespace npuvt::render
