if (TARGET SweetLine::sweetline_static)
    if (DEFINED sweetline_SOURCE_DIR AND NOT SWEETSHOT_3P_SWEETLINE_SOURCE_DIR)
        set(SWEETSHOT_3P_SWEETLINE_SOURCE_DIR ${sweetline_SOURCE_DIR})
    endif ()
    return()
endif ()

include(FetchContent)

set(SWEETLINE_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SWEETLINE_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(SWEETLINE_BUILD_STATIC ON CACHE BOOL "" FORCE)
set(SWEETLINE_BUILD_WASM_EMBIND OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        sweetline
        GIT_REPOSITORY https://github.com/FinalScave/SweetLine.git
        GIT_TAG f8aa883b50f2e748098420917fbef5865b55a861
        GIT_SUBMODULES ""
)
FetchContent_MakeAvailable(sweetline)

set(SWEETSHOT_3P_SWEETLINE_SOURCE_DIR ${sweetline_SOURCE_DIR})
