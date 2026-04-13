add_executable(trading_system_tests
    recovery_manager_test.cpp
    decoder_test.cpp
    sequencer_test.cpp
    orderbook_validation_test.cpp
    ${PROJECT_SOURCE_DIR}/src/binance/api.cpp
    ${PROJECT_SOURCE_DIR}/src/binance/decoder.cpp
    ${PROJECT_SOURCE_DIR}/src/binance/recovery_manager.cpp
    ${PROJECT_SOURCE_DIR}/src/binance/sequencer.cpp
    ${PROJECT_SOURCE_DIR}/src/common/symbol.cpp
    ${PROJECT_SOURCE_DIR}/src/utils/utils.cpp
)

target_include_directories(trading_system_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
)

target_link_libraries(trading_system_tests PRIVATE
    Boost::boost
    GTest::gtest_main
    GTest::gmock
    spdlog::spdlog
)

include(GoogleTest)
gtest_discover_tests(trading_system_tests)
