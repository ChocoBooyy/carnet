find_package(unofficial-sqlite3 CONFIG REQUIRED)

add_library(carnet_core STATIC
    src/money.cpp
    src/date.cpp
    src/csv.cpp
    src/import.cpp
    src/statement.cpp
    src/migrations.cpp
    src/database.cpp
)

target_include_directories(carnet_core PUBLIC include)
target_compile_features(carnet_core PUBLIC cxx_std_23)
target_link_libraries(carnet_core PRIVATE unofficial::sqlite3::sqlite3)
