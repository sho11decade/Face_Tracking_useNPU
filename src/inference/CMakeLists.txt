add_library(npuvt_inference_ov STATIC inference_service.cpp)
add_library(npuvt::inference ALIAS npuvt_inference_ov)

target_link_libraries(npuvt_inference_ov PUBLIC npuvt_project_options npuvt::core npuvt::models)
target_include_directories(npuvt_inference_ov PUBLIC ${PROJECT_SOURCE_DIR}/include)

if(NPUVT_ENABLE_OPENVINO)
    find_package(OpenVINO REQUIRED COMPONENTS Runtime)
    target_link_libraries(npuvt_inference_ov PUBLIC openvino::runtime)
endif()
