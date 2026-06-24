add_library(npuvt_capture STATIC capture_service.cpp)
add_library(npuvt::capture ALIAS npuvt_capture)

target_link_libraries(npuvt_capture PUBLIC npuvt_project_options npuvt::core)
target_include_directories(npuvt_capture PUBLIC ${PROJECT_SOURCE_DIR}/include)
