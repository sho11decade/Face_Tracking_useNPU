add_executable(npuvt_smoke_test smoke_test.cpp)

target_link_libraries(
    npuvt_smoke_test
    PRIVATE
        npuvt_project_options
        npuvt::capture
        npuvt::inference
        npuvt::tracking
        npuvt::mapping
)

add_test(NAME npuvt_smoke_test COMMAND npuvt_smoke_test)
