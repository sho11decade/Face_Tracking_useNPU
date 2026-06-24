add_library(npuvt_inference_ov STATIC inference_service.cpp)
add_library(npuvt::inference ALIAS npuvt_inference_ov)

target_link_libraries(npuvt_inference_ov PUBLIC npuvt_project_options npuvt::core)
target_include_directories(npuvt_inference_ov PUBLIC ${PROJECT_SOURCE_DIR}/include)
