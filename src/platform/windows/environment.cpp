#include "npu_vtube/platform/windows/environment.hpp"

namespace npuvt::platform::windows {

bool is_npu_driver_expected() noexcept {
#if defined(_WIN32)
    return true;
#else
    return false;
#endif
}

}  // namespace npuvt::platform::windows
