add_executable(sweetshot_smoke_tests smoke_test.cpp)
target_link_libraries(sweetshot_smoke_tests PRIVATE SweetShot::core)
target_compile_definitions(sweetshot_smoke_tests PRIVATE
        SWEETSHOT_TEST_SYNTAX_DIR="${sweetline_SOURCE_DIR}/syntaxes"
)

add_test(NAME SweetShotSmokeTests COMMAND sweetshot_smoke_tests)
