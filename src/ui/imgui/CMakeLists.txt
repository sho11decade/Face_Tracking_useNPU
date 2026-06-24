add_library(npuvt_ui_imgui STATIC debug_ui.cpp)
add_library(npuvt::ui_imgui ALIAS npuvt_ui_imgui)

target_link_libraries(npuvt_ui_imgui PUBLIC npuvt_project_options)
target_include_directories(npuvt_ui_imgui PUBLIC ${PROJECT_SOURCE_DIR}/include)
