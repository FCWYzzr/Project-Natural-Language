cmake_minimum_required(VERSION 3.29)
project(Project-Nature-Language)

set(CMAKE_CXX_STANDARD 23)
set_property(GLOBAL PROPERTY loc "${PROJECT_SOURCE_DIR}/Project-NL")

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DBG_DIR "debug")
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(DBG_DIR "release")
endif ()



foreach (dep "antlr4")
    file(GLOB dep-lib
        "${PROJECT_SOURCE_DIR}/dependency/${dep}/${DBG_DIR}/bin/*.lib"
    )
    file(GLOB dep-dlls
        "${PROJECT_SOURCE_DIR}/dependency/${dep}/${DBG_DIR}/*.dll"
    )
    file(GLOB_RECURSE dep-hs
        "${PROJECT_SOURCE_DIR}/dependency/${dep}/include/*"
    )
    set_property(GLOBAL PROPERTY "${dep}-lib" ${dep-lib})
    set_property(GLOBAL PROPERTY "${dep}-header" "${PROJECT_SOURCE_DIR}/dependency/${dep}/include")
    include_directories("${PROJECT_SOURCE_DIR}/dependency/${dep}/include")
    install(FILES ${dep-hs} DESTINATION "${loc}/include")
    install(FILES ${dep-dlls} DESTINATION "${loc}/LL")
endforeach ()

include_directories(dependency/dlfcn/include)

set_property(GLOBAL PROPERTY dep-includes ${includes})
set_property(GLOBAL PROPERTY dep-libs ${libs})


add_subdirectory(src/pnl/ll)



