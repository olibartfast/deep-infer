# Tests for the host-side deep-infer-core library and header-only utilities.
add_executable(deep-infer-tests
    test_utils.cpp
    test_config.cpp
    test_config_manager.cpp
    test_logger.cpp
    test_task_interface.cpp
    test_tensor_postprocess.cpp
)

target_link_libraries(deep-infer-tests
    PRIVATE
        deep-infer-core
        GTest::gtest_main
)

# Link the libjbig whole-archive workaround AFTER deep-infer-core so OpenCV (and
# its libtiff dependency) precedes libjbig on the link line. No-op when the
# static archive is absent.
if(TARGET deep-infer-jbig)
    target_link_libraries(deep-infer-tests
        PRIVATE
            deep-infer-jbig
    )
endif()

target_compile_options(deep-infer-tests
    PRIVATE
        -Wall
        -Wextra
        -Wno-unused-parameter
)

# Register each TEST()/TEST_F() as an individual CTest test.
gtest_discover_tests(deep-infer-tests)
