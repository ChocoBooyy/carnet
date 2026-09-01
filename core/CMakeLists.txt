add_library(carnet_core STATIC
    src/money.cpp
    src/date.cpp
)

target_include_directories(carnet_core PUBLIC include)
target_compile_features(carnet_core PUBLIC cxx_std_23)
