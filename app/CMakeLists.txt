set(CMAKE_AUTOMOC ON)

add_library(carnet_ui STATIC
    app_paths.cpp
    dialogs/about_dialog.cpp
    dialogs/column_roles.cpp
    dialogs/import_dialog.cpp
    dialogs/rules_dialog.cpp
    dialogs/text_decoding.cpp
    models/format.cpp
    models/transaction_filter.cpp
    models/transaction_table_model.cpp
    views/main_window.cpp
    views/overview_tab.cpp
    views/transactions_tab.cpp
    widgets/budget_bars.cpp
    widgets/category_delegate.cpp
    widgets/month_picker.cpp
    widgets/trend_chart.cpp
)

target_include_directories(carnet_ui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(carnet_ui PUBLIC carnet_core Qt6::Widgets)
target_compile_definitions(carnet_ui PUBLIC CARNET_VERSION="${PROJECT_VERSION}")

add_executable(carnet WIN32 main.cpp)

target_link_libraries(carnet PRIVATE carnet_ui)

# copy the Qt runtime next to carnet.exe so it starts from the build folder or Explorer
add_custom_command(TARGET carnet POST_BUILD
    COMMAND "${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS}/windeployqt.exe"
            --no-translations --no-system-d3d-compiler --no-opengl-sw "$<TARGET_FILE:carnet>"
    VERBATIM)
