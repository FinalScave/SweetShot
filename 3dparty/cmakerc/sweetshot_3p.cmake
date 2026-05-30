if (TARGET SweetShot3p::cmrc)
    return()
endif ()

include(FetchContent)

FetchContent_Declare(
        cmakerc
        GIT_REPOSITORY https://github.com/vector-of-bool/cmrc.git
        GIT_TAG 2.0.1
        GIT_SUBMODULES ""
)
if (POLICY CMP0169)
    cmake_policy(PUSH)
    cmake_policy(SET CMP0169 OLD)
endif ()
FetchContent_Populate(cmakerc)
if (POLICY CMP0169)
    cmake_policy(POP)
endif ()

set(SWEETSHOT_3P_CMAKERC_WARN_DEPRECATED ${CMAKE_WARN_DEPRECATED})
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "Show deprecated CMake warnings" FORCE)
include(${cmakerc_SOURCE_DIR}/CMakeRC.cmake)
set(CMAKE_WARN_DEPRECATED ${SWEETSHOT_3P_CMAKERC_WARN_DEPRECATED} CACHE BOOL "Show deprecated CMake warnings" FORCE)

add_library(SweetShot3p::cmrc ALIAS cmrc-base)
