cmake_minimum_required(VERSION 3.20)
project(trading_system LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Boost REQUIRED)
find_package(OpenSSL REQUIRED)

add_subdirectory(external/spdlog)
add_subdirectory(src)
