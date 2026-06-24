add_library(npuvt_filter STATIC one_euro_filter.cpp)
add_library(npuvt::filter ALIAS npuvt_filter)

target_link_libraries(npuvt_filter PUBLIC npuvt_project_options)
target_include_directories(npuvt_filter PUBLIC ${PROJECT_SOURCE_DIR}/include)
