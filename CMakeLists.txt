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
set(SWEETSHOT_PNG_BACKEND "resvg" CACHE STRING "PNG backend: resvg or lunasvg")
set_property(CACHE SWEETSHOT_PNG_BACKEND PROPERTY STRINGS resvg lunasvg)

if (SWEETSHOT_PNG_BACKEND STREQUAL "resvg")
    set(SWEETSHOT_PNG_BACKEND_SOURCE ${PROJECT_SOURCE_DIR}/src/resvg_rasterizer.cpp)
    set(SWEETSHOT_PNG_BACKEND_TARGET SweetShot3p::resvg)
    set(SWEETSHOT_PNG_BACKEND_DEFINITION SWEETSHOT_PNG_BACKEND_RESVG=1)
elseif (SWEETSHOT_PNG_BACKEND STREQUAL "lunasvg")
    set(SWEETSHOT_PNG_BACKEND_SOURCE ${PROJECT_SOURCE_DIR}/src/lunasvg_rasterizer.cpp)
    set(SWEETSHOT_PNG_BACKEND_TARGET SweetShot3p::lunasvg)
    set(SWEETSHOT_PNG_BACKEND_DEFINITION SWEETSHOT_PNG_BACKEND_LUNASVG=1)
else ()
    message(FATAL_ERROR "SWEETSHOT_PNG_BACKEND must be 'resvg' or 'lunasvg'")
endif ()

include(cmake/SweetShotThirdParty.cmake)

set(SWEETSHOT_PUBLIC_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(SWEETSHOT_SRC_DIR ${PROJECT_SOURCE_DIR}/src)

add_library(sweetshot_core STATIC
        ${SWEETSHOT_SRC_DIR}/renderer.cpp
        ${SWEETSHOT_PNG_BACKEND_SOURCE}
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
target_compile_definitions(sweetshot_core PUBLIC
        ${SWEETSHOT_PNG_BACKEND_DEFINITION}
)
target_link_libraries(sweetshot_core PUBLIC
        SweetLine::sweetline_static
)
target_link_libraries(sweetshot_core PRIVATE
        SweetShot3p::lodepng
        ${SWEETSHOT_PNG_BACKEND_TARGET}
)

if (MSVC)
    target_compile_options(sweetshot_core PRIVATE /Zc:__cplusplus /utf-8)
    target_compile_definitions(sweetshot_core PRIVATE _CRT_SECURE_NO_WARNINGS NOMINMAX=1)
endif ()

if (SWEETSHOT_BUILD_CLI)
    add_executable(sweetshot cli/main.cpp)
    target_link_libraries(sweetshot PRIVATE SweetShot::core)
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
