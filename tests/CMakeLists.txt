add_executable(trading_system_tests
    smoke_test.cpp
)

target_link_libraries(trading_system_tests
    PRIVATE
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(trading_system_tests)
