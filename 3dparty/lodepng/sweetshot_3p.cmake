if (TARGET SweetShot3p::lodepng)
    return()
endif ()

include(FetchContent)

FetchContent_Declare(
        lodepng
        GIT_REPOSITORY https://github.com/lvandeve/lodepng.git
        GIT_TAG ed6fe5825c6a4fbb7f58ab35a4231c7543cd452a
        GIT_SUBMODULES ""
)
if (POLICY CMP0169)
    cmake_policy(PUSH)
    cmake_policy(SET CMP0169 OLD)
endif ()
FetchContent_Populate(lodepng)
if (POLICY CMP0169)
    cmake_policy(POP)
endif ()

add_library(sweetshot_3p_lodepng STATIC
        ${lodepng_SOURCE_DIR}/lodepng.cpp
)
add_library(SweetShot3p::lodepng ALIAS sweetshot_3p_lodepng)

target_include_directories(sweetshot_3p_lodepng
        PUBLIC ${lodepng_SOURCE_DIR}
)

if (MSVC)
    target_compile_options(sweetshot_3p_lodepng PRIVATE /utf-8)
endif ()
