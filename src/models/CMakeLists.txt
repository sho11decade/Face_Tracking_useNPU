add_library(npuvt_models STATIC model_catalog.cpp model_resolver.cpp)
add_library(npuvt::models ALIAS npuvt_models)

target_link_libraries(npuvt_models PUBLIC npuvt_project_options)
target_include_directories(npuvt_models PUBLIC ${PROJECT_SOURCE_DIR}/include)
