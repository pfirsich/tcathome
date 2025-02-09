FetchContent_Declare(
    tinycc
    GIT_REPOSITORY https://github.com/TinyCC/tinycc.git
    GIT_TAG f6385c05308f715bdd2c06336801193a21d69b50
)
FetchContent_MakeAvailable(tinycc)

FetchContent_GetProperties(tinycc SOURCE_DIR TCC_SOURCE_DIR)

set(TCC_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/tinycc)
set(TCC_STATIC_LIB ${TCC_BINARY_DIR}/libtcc.a)
set(TCC_RUNTIME_LIB ${TCC_BINARY_DIR}/libtcc1.a)
set(TCC_INCLUDE_DIR ${TCC_SOURCE_DIR})

add_library(tcc STATIC IMPORTED GLOBAL)
set_target_properties(tcc PROPERTIES
    IMPORTED_LOCATION ${TCC_STATIC_LIB}
    INTERFACE_INCLUDE_DIRECTORIES "${TCC_INCLUDE_DIR};${TCC_BINARY_DIR}" # binary dir for generated config.h
)

set(TCC_BUILD_FLAGS "")
if(CMAKE_BUILD_TYPE STREQUAL "Debug" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(TCC_BUILD_FLAGS "-g")
endif()

file(MAKE_DIRECTORY ${TCC_BINARY_DIR})

file(GLOB TCC_SOURCES
    ${TCC_SOURCE_DIR}/*.c
    ${TCC_SOURCE_DIR}/*.h
    ${TCC_SOURCE_DIR}/configure
    ${TCC_SOURCE_DIR}/Makefile*
)

add_custom_command(
    OUTPUT ${TCC_STATIC_LIB} ${TCC_RUNTIME_LIB}
    # I don't know why I need to call into CMake here again, but it seems CFLAGS will
    # not be quoted correctly otherwise. Claude told me to do this and I don't even want
    # to know how it works. The more you know about CMake, the dumber you get.
    #COMMAND ${TCC_SOURCE_DIR}/configure
    COMMAND ${CMAKE_COMMAND} -E env "CFLAGS=${TCC_BUILD_FLAGS}"
            ${TCC_SOURCE_DIR}/configure
    #COMMAND make CFLAGS="${TCC_BUILD_FLAGS}" libtcc.a libtcc1.a
    COMMAND ${CMAKE_COMMAND} -E env "CFLAGS=${TCC_BUILD_FLAGS}"
            make libtcc.a libtcc1.a
    WORKING_DIRECTORY ${TCC_BINARY_DIR}
    DEPENDS ${TCC_SOURCES}
    COMMENT "Building libtcc"
    VERBATIM

)

add_custom_target(tcc_build
    DEPENDS ${TCC_STATIC_LIB}
)

add_dependencies(tcc tcc_build)
