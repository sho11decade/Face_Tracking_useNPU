add_executable(npuvt_app_dev main.cpp)
add_executable(npuvt::app_dev ALIAS npuvt_app_dev)

target_link_libraries(
    npuvt_app_dev
    PRIVATE
        npuvt_project_options
        npuvt::core
        npuvt::capture
        npuvt::inference
        npuvt::tracking
        npuvt::mapping
        npuvt::render_dx11
        npuvt::ui_imgui
        npuvt::platform_windows
)

set_target_properties(npuvt_app_dev PROPERTIES OUTPUT_NAME tracker_dev)
