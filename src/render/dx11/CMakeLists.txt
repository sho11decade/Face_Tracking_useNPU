add_library(npuvt_render_dx11 STATIC render_backend.cpp)
add_library(npuvt::render_dx11 ALIAS npuvt_render_dx11)

target_link_libraries(npuvt_render_dx11 PUBLIC npuvt_project_options)
target_include_directories(npuvt_render_dx11 PUBLIC ${PROJECT_SOURCE_DIR}/include)
