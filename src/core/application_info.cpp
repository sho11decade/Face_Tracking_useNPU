#include "npu_vtube/core/application_info.hpp"

namespace npuvt::core {

ApplicationInfo make_application_info() noexcept {
    return {
        .name = "NPU_Vtube",
        .version = "0.1.0",
        .description = "Real-time face tracking skeleton for Intel AI Boost NPU.",
    };
}

}  // namespace npuvt::core
