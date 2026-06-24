#pragma once

#include <string_view>

namespace npuvt::core {

struct ApplicationInfo {
    std::string_view name;
    std::string_view version;
    std::string_view description;
};

ApplicationInfo make_application_info() noexcept;

}  // namespace npuvt::core
