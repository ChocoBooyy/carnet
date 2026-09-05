set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Test)

function(carnet_qt_test name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE carnet_ui Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
    set_tests_properties(${name} PROPERTIES ENVIRONMENT_MODIFICATION
        "PATH=path_list_prepend:${QT6_INSTALL_PREFIX}/${QT6_INSTALL_BINS};QT_QPA_PLATFORM=set:offscreen")
endfunction()

carnet_qt_test(format_test)
carnet_qt_test(transaction_table_model_test)
carnet_qt_test(transaction_filter_test)
carnet_qt_test(text_decoding_test)
carnet_qt_test(column_roles_test)
