cmake_minimum_required(VERSION 3.25)

project(Carnet VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4 /utf-8 /permissive-)
endif()

option(CARNET_BUILD_TESTS "Build the core unit tests" ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_subdirectory(core)
add_subdirectory(app)

if(CARNET_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
