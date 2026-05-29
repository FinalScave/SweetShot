cmake_minimum_required(VERSION 3.16)
project(sweetshot VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin)

if (PROJECT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    set(SWEETSHOT_DEFAULT_BUILD_CLI ON)
    set(SWEETSHOT_DEFAULT_BUILD_TESTS ON)
else ()
    set(SWEETSHOT_DEFAULT_BUILD_CLI OFF)
    set(SWEETSHOT_DEFAULT_BUILD_TESTS OFF)
endif ()

option(SWEETSHOT_BUILD_CLI "Build SweetShot command line tool" ${SWEETSHOT_DEFAULT_BUILD_CLI})
option(SWEETSHOT_BUILD_TESTS "Build SweetShot tests" ${SWEETSHOT_DEFAULT_BUILD_TESTS})

include(cmake/SweetShotThirdParty.cmake)

set(SWEETSHOT_PUBLIC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(SWEETSHOT_SRC_DIR ${PROJECT_SOURCE_DIR}/src)

add_library(sweetshot_core STATIC
        ${SWEETSHOT_SRC_DIR}/renderer.cpp
        ${SWEETSHOT_SRC_DIR}/resvg_rasterizer.cpp
        ${SWEETSHOT_SRC_DIR}/theme.cpp
)
add_library(SweetShot::core ALIAS sweetshot_core)

target_compile_features(sweetshot_core PUBLIC cxx_std_17)
target_include_directories(sweetshot_core
        PUBLIC
        $<BUILD_INTERFACE:${SWEETSHOT_PUBLIC_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
)
target_compile_definitions(sweetshot_core PRIVATE
        SWEETSHOT_DEFAULT_SYNTAX_DIR="${SWEETSHOT_3P_SWEETLINE_SOURCE_DIR}/syntaxes"
)
target_link_libraries(sweetshot_core PUBLIC
        SweetLine::sweetline_static
        SweetShot3p::resvg
)
target_link_libraries(sweetshot_core PRIVATE SweetShot3p::lodepng)

if (MSVC)
    target_compile_options(sweetshot_core PRIVATE /Zc:__cplusplus /utf-8)
    target_compile_definitions(sweetshot_core PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX=1)
endif ()

if (SWEETSHOT_BUILD_CLI)
    add_executable(sweetshot cli/main.cpp)
    target_link_libraries(sweetshot PRIVATE SweetShot::core)
    if (WIN32)
        add_custom_command(TARGET sweetshot POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:SweetShot3p::resvg>
                $<TARGET_FILE_DIR:sweetshot>
        )
    endif ()
endif ()

if (SWEETSHOT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif ()

include(GNUInstallDirs)
install(TARGETS sweetshot_core
        EXPORT SweetShotTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if (TARGET sweetshot)
    install(TARGETS sweetshot RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif ()

install(DIRECTORY ${SWEETSHOT_PUBLIC_INCLUDE_DIR}/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h"
)
