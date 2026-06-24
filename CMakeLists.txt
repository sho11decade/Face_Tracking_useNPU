cmake_minimum_required(VERSION 3.25)

project(NPU_Vtube VERSION 0.1.0 LANGUAGES CXX)

include(CTest)

option(NPUVT_BUILD_APP "Build the development application" ON)
option(NPUVT_BUILD_TESTS "Build tests" ON)
option(NPUVT_ENABLE_OPENVINO "Enable OpenVINO integration" OFF)
option(NPUVT_ENABLE_OPENCV "Enable OpenCV integration" OFF)
option(NPUVT_ENABLE_IMGUI "Enable Dear ImGui integration" OFF)
option(NPUVT_ENABLE_TBB "Enable oneTBB integration" OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(npuvt_project_options INTERFACE)
target_compile_features(npuvt_project_options INTERFACE cxx_std_20)

if(MSVC)
    target_compile_options(npuvt_project_options INTERFACE /W4 /permissive- /Zc:__cplusplus)
else()
    target_compile_options(npuvt_project_options INTERFACE -Wall -Wextra -Wpedantic)
endif()

target_compile_definitions(
    npuvt_project_options
    INTERFACE
        $<$<BOOL:${NPUVT_ENABLE_OPENVINO}>:NPUVT_ENABLE_OPENVINO>
        $<$<BOOL:${NPUVT_ENABLE_OPENCV}>:NPUVT_ENABLE_OPENCV>
        $<$<BOOL:${NPUVT_ENABLE_IMGUI}>:NPUVT_ENABLE_IMGUI>
        $<$<BOOL:${NPUVT_ENABLE_TBB}>:NPUVT_ENABLE_TBB>
)

add_subdirectory(src)

if(NPUVT_BUILD_APP)
    add_subdirectory(apps/tracker_dev)
endif()

if(BUILD_TESTING AND NPUVT_BUILD_TESTS)
    add_subdirectory(tests)
endif()
