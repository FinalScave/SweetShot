set(TEST_PRODUCT_NAME sweetshot_tests)

add_executable(${TEST_PRODUCT_NAME}
        tests_main.cpp
        smoke_test.cpp
        visual_regression_test.cpp
)

target_compile_features(${TEST_PRODUCT_NAME} PRIVATE cxx_std_17)
target_link_libraries(${TEST_PRODUCT_NAME} PRIVATE
        SweetShot::core
        SweetShot3p::Catch2
)
target_compile_definitions(${TEST_PRODUCT_NAME} PRIVATE
        SWEETSHOT_TEST_SYNTAX_DIR="${SWEETSHOT_3P_SWEETLINE_SOURCE_DIR}/syntaxes"
)
if (MSVC)
    target_compile_options(${TEST_PRODUCT_NAME} PRIVATE /Zc:__cplusplus /utf-8)
    target_compile_definitions(${TEST_PRODUCT_NAME} PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX=1)
endif ()

if (WIN32 AND SWEETSHOT_PNG_BACKEND STREQUAL "resvg")
    add_custom_command(TARGET ${TEST_PRODUCT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SweetShot3p::resvg>
            $<TARGET_FILE_DIR:${TEST_PRODUCT_NAME}>
    )
endif ()

add_test(NAME SweetShotTests COMMAND ${TEST_PRODUCT_NAME})
