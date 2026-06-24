add_library(npuvt_tracking STATIC tracking_service.cpp)
add_library(npuvt::tracking ALIAS npuvt_tracking)

target_link_libraries(npuvt_tracking PUBLIC npuvt_project_options npuvt::core npuvt::filter)
target_include_directories(npuvt_tracking PUBLIC ${PROJECT_SOURCE_DIR}/include)
