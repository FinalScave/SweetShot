if (TARGET SweetShot3p::lunasvg)
    return()
endif ()

include(FetchContent)

FetchContent_Declare(
        lunasvg
        GIT_REPOSITORY https://github.com/sammycage/lunasvg.git
        GIT_TAG 2872affa1027cad92a05408a7e6f2547efa7f364
        GIT_SUBMODULES ""
)
if (POLICY CMP0169)
    cmake_policy(PUSH)
    cmake_policy(SET CMP0169 OLD)
endif ()
FetchContent_Populate(lunasvg)
if (POLICY CMP0169)
    cmake_policy(POP)
endif ()

set(SWEETSHOT_3P_LUNASVG_SOURCE_DIR ${lunasvg_SOURCE_DIR})
set(LUNASVG_BUILD_EXAMPLES OFF CACHE BOOL "Disable LunaSVG examples" FORCE)
set(PLUTOVG_BUILD_EXAMPLES OFF CACHE BOOL "Disable PlutoVG examples" FORCE)
set(USE_SYSTEM_PLUTOVG OFF CACHE BOOL "Use bundled PlutoVG" FORCE)

if (DEFINED BUILD_SHARED_LIBS)
    set(SWEETSHOT_3P_LUNASVG_OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
    set(SWEETSHOT_3P_LUNASVG_HAD_BUILD_SHARED_LIBS ON)
endif ()
set(BUILD_SHARED_LIBS OFF)

add_subdirectory(${lunasvg_SOURCE_DIR} ${lunasvg_BINARY_DIR} EXCLUDE_FROM_ALL)
add_library(SweetShot3p::lunasvg ALIAS lunasvg)

if (SWEETSHOT_3P_LUNASVG_HAD_BUILD_SHARED_LIBS)
    set(BUILD_SHARED_LIBS ${SWEETSHOT_3P_LUNASVG_OLD_BUILD_SHARED_LIBS})
else ()
    unset(BUILD_SHARED_LIBS)
endif ()
