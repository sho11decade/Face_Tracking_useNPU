#pragma once

#include <string_view>

namespace npuvt::ui {

class DebugUi {
public:
    [[nodiscard]] std::string_view name() const noexcept;
};

}  // namespace npuvt::ui
