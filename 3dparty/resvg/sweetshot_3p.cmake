if (TARGET SweetShot3p::resvg)
    return()
endif ()

include(FetchContent)
find_package(Threads REQUIRED)

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
set(SWEETSHOT_3P_RESVG_CARGO_TARGET "")
set(SWEETSHOT_3P_RESVG_CARGO_ENV CARGO_TARGET_DIR=${SWEETSHOT_3P_RESVG_TARGET_DIR})
set(SWEETSHOT_3P_RESVG_CARGO_ARGS build --release)

if (APPLE)
    set(SWEETSHOT_3P_RESVG_APPLE_ARCH ${CMAKE_OSX_ARCHITECTURES})
    if (NOT SWEETSHOT_3P_RESVG_APPLE_ARCH)
        set(SWEETSHOT_3P_RESVG_APPLE_ARCH ${CMAKE_SYSTEM_PROCESSOR})
    endif ()
    if (SWEETSHOT_3P_RESVG_APPLE_ARCH MATCHES "^(arm64|aarch64)$")
        set(SWEETSHOT_3P_RESVG_CARGO_TARGET aarch64-apple-darwin)
    elseif (SWEETSHOT_3P_RESVG_APPLE_ARCH MATCHES "^(x86_64|amd64)$")
        set(SWEETSHOT_3P_RESVG_CARGO_TARGET x86_64-apple-darwin)
    else ()
        message(FATAL_ERROR "Unsupported macOS resvg architecture: ${SWEETSHOT_3P_RESVG_APPLE_ARCH}")
    endif ()
elseif (UNIX)
    string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SWEETSHOT_3P_RESVG_SYSTEM_PROCESSOR)
    if (SWEETSHOT_3P_RESVG_SYSTEM_PROCESSOR MATCHES "^(aarch64|arm64)$")
        set(SWEETSHOT_3P_RESVG_CARGO_TARGET aarch64-unknown-linux-gnu)
        list(APPEND SWEETSHOT_3P_RESVG_CARGO_ENV
                CARGO_TARGET_AARCH64_UNKNOWN_LINUX_GNU_LINKER=aarch64-linux-gnu-gcc
        )
    elseif (SWEETSHOT_3P_RESVG_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
        set(SWEETSHOT_3P_RESVG_CARGO_TARGET x86_64-unknown-linux-gnu)
    else ()
        message(FATAL_ERROR "Unsupported Linux resvg architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif ()
endif ()

if (SWEETSHOT_3P_RESVG_CARGO_TARGET)
    list(APPEND SWEETSHOT_3P_RESVG_CARGO_ARGS --target ${SWEETSHOT_3P_RESVG_CARGO_TARGET})
    set(SWEETSHOT_3P_RESVG_BUILD_DIR ${SWEETSHOT_3P_RESVG_TARGET_DIR}/${SWEETSHOT_3P_RESVG_CARGO_TARGET}/release)
else ()
    set(SWEETSHOT_3P_RESVG_BUILD_DIR ${SWEETSHOT_3P_RESVG_TARGET_DIR}/release)
endif ()

if (WIN32)
    set(SWEETSHOT_3P_RESVG_LIBRARY ${SWEETSHOT_3P_RESVG_BUILD_DIR}/resvg.lib)
elseif (APPLE)
    set(SWEETSHOT_3P_RESVG_LIBRARY ${SWEETSHOT_3P_RESVG_BUILD_DIR}/libresvg.a)
else ()
    set(SWEETSHOT_3P_RESVG_LIBRARY ${SWEETSHOT_3P_RESVG_BUILD_DIR}/libresvg.a)
endif ()

add_custom_command(
        OUTPUT ${SWEETSHOT_3P_RESVG_LIBRARY}
        COMMAND ${CMAKE_COMMAND} -E env
                ${SWEETSHOT_3P_RESVG_CARGO_ENV}
                ${SWEETSHOT_3P_CARGO_EXECUTABLE} ${SWEETSHOT_3P_RESVG_CARGO_ARGS}
                --manifest-path ${SWEETSHOT_3P_RESVG_SOURCE_DIR}/crates/c-api/Cargo.toml
        WORKING_DIRECTORY ${SWEETSHOT_3P_RESVG_SOURCE_DIR}
        COMMENT "Building resvg C API"
        VERBATIM
)

add_custom_target(SweetShot3pResvgBuild DEPENDS ${SWEETSHOT_3P_RESVG_LIBRARY})

add_library(SweetShot3p::resvg STATIC IMPORTED GLOBAL)
set_target_properties(SweetShot3p::resvg PROPERTIES
        IMPORTED_LOCATION ${SWEETSHOT_3P_RESVG_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${SWEETSHOT_3P_RESVG_SOURCE_DIR}/crates/c-api
)

if (WIN32)
    set_target_properties(SweetShot3p::resvg PROPERTIES
            INTERFACE_LINK_LIBRARIES "ntdll;userenv"
    )
elseif (UNIX AND NOT APPLE)
    set_target_properties(SweetShot3p::resvg PROPERTIES
            INTERFACE_LINK_LIBRARIES "Threads::Threads;${CMAKE_DL_LIBS};m"
    )
endif ()

add_dependencies(SweetShot3p::resvg SweetShot3pResvgBuild)
