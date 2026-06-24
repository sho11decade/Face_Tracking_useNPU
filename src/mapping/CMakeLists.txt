add_library(npuvt_mapping STATIC parameter_mapper.cpp)
add_library(npuvt::mapping ALIAS npuvt_mapping)

target_link_libraries(npuvt_mapping PUBLIC npuvt_project_options npuvt::core)
target_include_directories(npuvt_mapping PUBLIC ${PROJECT_SOURCE_DIR}/include)
