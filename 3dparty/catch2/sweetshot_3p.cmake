if (TARGET SweetShot3p::Catch2)
    return()
endif ()

add_library(sweetshot_3p_catch2
        ${CMAKE_CURRENT_LIST_DIR}/src/catch_amalgamated.cpp
)

target_compile_features(sweetshot_3p_catch2 PUBLIC cxx_std_17)
target_compile_definitions(sweetshot_3p_catch2 PUBLIC CATCH_AMALGAMATED_CUSTOM_MAIN=ON)

target_include_directories(sweetshot_3p_catch2
        PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
        PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/include/catch2
)

add_library(SweetShot3p::Catch2 ALIAS sweetshot_3p_catch2)
