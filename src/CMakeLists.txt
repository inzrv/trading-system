file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
    "*.cpp"
)

add_executable(trading_system ${PROJECT_SOURCES})

target_include_directories(trading_system
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${Boost_INCLUDE_DIRS}
)

target_link_libraries(trading_system
    PRIVATE
        Boost::boost
        OpenSSL::SSL
        OpenSSL::Crypto
        spdlog::spdlog
)

if(APPLE)
    target_compile_options(trading_system PRIVATE -Wall -Wextra -Wpedantic)
endif()
