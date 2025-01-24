project(LowerLevel C CXX)

get_property(dependencies GLOBAL PROPERTY dependencies)
foreach (dep ${dependencies})
    get_property(${dep} GLOBAL PROPERTY "${dep}")
    get_property("${dep}-headers" GLOBAL PROPERTY "${dep}-headers")
endforeach ()


# PNL Lib
file(GLOB_RECURSE PNL-Lower-Level-interface
    lib/*.cppm
    lib/*.h
)

file(GLOB_RECURSE PNL-Lower-Level-impl
    lib/*.cpp
)

set(LIB-DIR "${loc}")
set(TGT-DIR ${LIB-DIR})
add_library(nl-ll SHARED
    ${PNL-Lower-Level-interface}
    ${PNL-Lower-Level-impl}
)
target_link_libraries(nl-ll
    ${dlfcn}
    ${libiconv}
)
target_include_directories(nl-ll PUBLIC
    ${PNL-Include}
    ${dlfcn-headers}
    ${libiconv-headers}
)
target_compile_definitions(nl-ll PUBLIC
    "PNL_LIB_PREFIX=${EXPORT}"
)


add_subdirectory(builtins)


file(GLOB_RECURSE nlsm-src
    nlsm/*.cpp
    nlsm/*.cppm
)
add_executable(nlsm
    ${nlsm-src}
    ${PNL-Lower-Level-interface}
)
target_include_directories(nlsm PUBLIC
    nlsm/antlr_gen
    ${PNL-Include}
    ${dlfcn-headers}
    ${libiconv-headers}
    ${antlr4-headers}
)
target_link_libraries(nlsm
    nl-ll
    ${antlr4}
)
target_compile_definitions(nlsm PUBLIC
    "PNL_LIB_PREFIX=${IMPORT}"
)


file(GLOB_RECURSE nlim-src
    nlim/*.cpp
    nlim/*.cppm
)
add_executable(nlim
    ${nlim-src}
    ${PNL-Lower-Level-interface}
)
target_include_directories(nlim PUBLIC
    ${PNL-Include}
    ${dlfcn-headers}
    ${libiconv-headers}
    ${antlr4-headers}
)
target_link_libraries(nlim
    nl-ll
    ${antlr4}
)
target_compile_definitions(nlim PUBLIC
    "PNL_LIB_PREFIX=${IMPORT}"
)


file(GLOB_RECURSE nlvm-common-src
    nlvm/common/*.cpp
    nlvm/common/*.cppm
)

add_compile_definitions(
    ASM_EXT=".nlsm"
    PKG_EXT=".nlpkg"
    IMG_EXT=".nlimg"
)

if (${CMAKE_CXX_PLATFORM_ID} STREQUAL Windows)
    add_subdirectory(nlvm/win32)
#elseif()

endif ()

install(FILES ${PNL-Lower-Level-interface} DESTINATION "${loc}/module/ll")
install(TARGETS nl-ll
    RUNTIME DESTINATION "${loc}/bin"
    LIBRARY DESTINATION "${loc}/lib")
install(TARGETS nlsm DESTINATION "${loc}/bin")
