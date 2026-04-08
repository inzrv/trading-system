add_executable(trading_system_tests
    recovery_manager_test.cpp
    ${PROJECT_SOURCE_DIR}/src/binance/recovery_manager.cpp
)

target_include_directories(trading_system_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(trading_system_tests PRIVATE
    GTest::gtest_main
    GTest::gmock
    spdlog::spdlog
)

include(GoogleTest)
gtest_discover_tests(trading_system_tests)
