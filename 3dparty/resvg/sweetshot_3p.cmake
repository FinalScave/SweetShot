if (TARGET SweetShot3p::resvg)
    return()
endif ()

include(FetchContent)

find_program(SWEETSHOT_3P_CARGO_EXECUTABLE cargo REQUIRED)

FetchContent_Declare(
        resvg
        GIT_REPOSITORY https://github.com/linebender/resvg.git
        GIT_TAG v0.47.0
        GIT_SUBMODULES ""
)
if (POLICY CMP0169)
    cmake_policy(PUSH)
    cmake_policy(SET CMP0169 OLD)
endif ()
FetchContent_Populate(resvg)
if (POLICY CMP0169)
    cmake_policy(POP)
endif ()

set(SWEETSHOT_3P_RESVG_SOURCE_DIR ${resvg_SOURCE_DIR})
set(SWEETSHOT_3P_RESVG_TARGET_DIR ${CMAKE_BINARY_DIR}/_deps/resvg-cargo-target)
set(SWEETSHOT_3P_RESVG_BUILD_DIR ${SWEETSHOT_3P_RESVG_TARGET_DIR}/release)

if (WIN32)
    set(SWEETSHOT_3P_RESVG_LIBRARY ${SWEETSHOT_3P_RESVG_BUILD_DIR}/resvg.dll.lib)
    set(SWEETSHOT_3P_RESVG_RUNTIME ${SWEETSHOT_3P_RESVG_BUILD_DIR}/resvg.dll)
elseif (APPLE)
    set(SWEETSHOT_3P_RESVG_LIBRARY ${SWEETSHOT_3P_RESVG_BUILD_DIR}/libresvg.dylib)
    set(SWEETSHOT_3P_RESVG_RUNTIME ${SWEETSHOT_3P_RESVG_LIBRARY})
else ()
    set(SWEETSHOT_3P_RESVG_LIBRARY ${SWEETSHOT_3P_RESVG_BUILD_DIR}/libresvg.so)
    set(SWEETSHOT_3P_RESVG_RUNTIME ${SWEETSHOT_3P_RESVG_LIBRARY})
endif ()

add_custom_command(
        OUTPUT ${SWEETSHOT_3P_RESVG_LIBRARY}
        BYPRODUCTS ${SWEETSHOT_3P_RESVG_RUNTIME}
        COMMAND ${CMAKE_COMMAND} -E env
                CARGO_TARGET_DIR=${SWEETSHOT_3P_RESVG_TARGET_DIR}
                ${SWEETSHOT_3P_CARGO_EXECUTABLE} build --release
                --manifest-path ${SWEETSHOT_3P_RESVG_SOURCE_DIR}/crates/c-api/Cargo.toml
        WORKING_DIRECTORY ${SWEETSHOT_3P_RESVG_SOURCE_DIR}
        COMMENT "Building resvg C API"
        VERBATIM
)

add_custom_target(SweetShot3pResvgBuild DEPENDS ${SWEETSHOT_3P_RESVG_LIBRARY})

add_library(SweetShot3p::resvg SHARED IMPORTED GLOBAL)
set_target_properties(SweetShot3p::resvg PROPERTIES
        IMPORTED_LOCATION ${SWEETSHOT_3P_RESVG_RUNTIME}
        INTERFACE_INCLUDE_DIRECTORIES ${SWEETSHOT_3P_RESVG_SOURCE_DIR}/crates/c-api
)

if (WIN32)
    set_target_properties(SweetShot3p::resvg PROPERTIES
            IMPORTED_IMPLIB ${SWEETSHOT_3P_RESVG_LIBRARY}
    )
endif ()

add_dependencies(SweetShot3p::resvg SweetShot3pResvgBuild)
