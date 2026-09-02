find_package(GTest CONFIG REQUIRED)

add_executable(carnet_tests
    money_test.cpp
    date_test.cpp
    csv_test.cpp
    import_test.cpp
)

target_link_libraries(carnet_tests PRIVATE carnet_core GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(carnet_tests)
