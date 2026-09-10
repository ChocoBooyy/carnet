find_package(GTest CONFIG REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_executable(carnet_tests
    money_test.cpp
    date_test.cpp
    csv_test.cpp
    import_test.cpp
    database_test.cpp
    dedup_test.cpp
    rules_test.cpp
    categories_test.cpp
    categorize_test.cpp
    overview_query_test.cpp
)

target_link_libraries(carnet_tests PRIVATE carnet_core GTest::gtest_main unofficial::sqlite3::sqlite3)
target_compile_definitions(carnet_tests PRIVATE CARNET_TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
target_include_directories(carnet_tests PRIVATE ${PROJECT_SOURCE_DIR}/core/src)

include(GoogleTest)
gtest_discover_tests(carnet_tests)

add_subdirectory(qt)
