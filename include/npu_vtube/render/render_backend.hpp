#pragma once

#include <string_view>

namespace npuvt::render {

class RenderBackend {
public:
    [[nodiscard]] std::string_view name() const noexcept;
};

}  // namespace npuvt::render
