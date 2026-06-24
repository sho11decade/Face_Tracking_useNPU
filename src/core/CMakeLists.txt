add_library(npuvt_core STATIC application_info.cpp)
add_library(npuvt::core ALIAS npuvt_core)

target_link_libraries(npuvt_core PUBLIC npuvt_project_options)
target_include_directories(npuvt_core PUBLIC ${PROJECT_SOURCE_DIR}/include)
