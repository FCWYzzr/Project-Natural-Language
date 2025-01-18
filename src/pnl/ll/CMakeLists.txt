project(LowerLevel C CXX)

get_property(loc GLOBAL PROPERTY loc)
get_property(includes GLOBAL PROPERTY dep-includes)
get_property(libs GLOBAL PROPERTY dep-libs)
get_property(antlr4-header GLOBAL PROPERTY antlr4-header)
get_property(antlr4-lib GLOBAL PROPERTY antlr4-lib)


# PNL Lib

file(GLOB_RECURSE PNL-Lower-Level-interface
    lib/*.cppm
)

file(GLOB_RECURSE PNL-Lower-Level-impl
    lib/*.cpp
    lib/*.c
)

set(LIB-DIR "${loc}")
set(TGT-DIR ${LIB-DIR})
add_library(NL_LL SHARED
    ${PNL-Lower-Level-interface}
    ${PNL-Lower-Level-impl}
)
target_compile_definitions(NL_LL PUBLIC
    DLFCN_WIN32_SHARED
    DLFCN_WIN32_EXPORTS
    PNL_LIB_PREFIX=__declspec\(dllexport\)
)
if (${CMAKE_CXX_COMPILER} STREQUAL "MSVC")
    link_options(
        /NXCOMPAT:NO
    )
endif ()

add_subdirectory(builtins)


file(GLOB_RECURSE nlsm-src
    nlsm/*.cpp
    nlsm/*.cppm
)
include_directories(nlsm/antlr_gen)
add_executable(NLSM
    ${nlsm-src}
    ${PNL-Lower-Level-interface}
)
target_link_libraries(NLSM
    NL_LL
    ${antlr4-lib}
)
add_compile_definitions(
    DLFCN_WIN32_SHARED
)



file(GLOB_RECURSE nlvm-src
    nlvm/*.cpp
    nlvm/*.cppm
)
add_executable(NLVM
    ${nlvm-src}
    ${PNL-Lower-Level-interface}
)
target_link_libraries(NLVM
    NL_LL
)



add_compile_definitions(
    ASM_EXT=L".nlsm"
    PKG_EXT=L".nlpkg"
    IMG_EXT=L".nlimg"
)

install(FILES ${PNL-Lower-Level-interface} DESTINATION "${loc}/module/ll")
install(TARGETS NL_LL DESTINATION "${loc}/LL")
install(TARGETS NLSM DESTINATION "${loc}/LL")
install(TARGETS NLVM DESTINATION "${loc}/LL")