set(CMAKE_AUTOMOC ON)

add_executable(carnet_screenshots main.cpp)
target_link_libraries(carnet_screenshots PRIVATE carnet_ui)
target_compile_definitions(carnet_screenshots PRIVATE CARNET_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
