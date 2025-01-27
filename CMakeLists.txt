cmake_minimum_required(VERSION 3.28)
project(Project-Nature-Language)

set(CMAKE_CXX_STANDARD 23)
set(loc "${PROJECT_SOURCE_DIR}/Project-NL")



set(ASM_EXT ".nlsm" CACHE STRING "pnl asm extension name")
set(PKG_EXT ".nlpkg" CACHE STRING "pnl standalone package extension name")
set(IMG_EXT ".nlimg" CACHE STRING "pnl precompiled packed package (a.k.a. image) extension name (load order shall be the same)")
option(STRICT_MEMORY_MANAGEMENT "force std::pmr use process memory to alloc instead of new/delete." ON)


if (${CMAKE_CXX_PLATFORM_ID} STREQUAL "Windows")
    set(lib_ext "lib")
    set(dll_ext "dll")
    set(linker_ext "lib")
elseif (${CMAKE_CXX_PLATFORM_ID} STREQUAL "Linux")
    set(lib_ext "a")
    set(dll_ext "so")
    set(linker_ext "so")
# elseif(${CMAKE_CXX_PLATFORM_ID} STREQUAL "Apple")
else ()
    message(FATAL_ERROR "please configure your platform properties manually: ${CMAKE_CXX_PLATFORM_ID}")
endif ()


if (${CMAKE_CXX_COMPILER_ID} STREQUAL MSVC)
    set(EXPORT "__declspec(dllexport)")
    set(IMPORT "__declspec(dllimport)")
else()
    message(WARNING "until 2025-1-21, compilers other than MSVC DO NOT fully support module features. You may use clang++/MSVC instead.")
    message(WARNING ">>>>>> YOU HAVE BEEN WARNED <<<<<<")

    if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
        set(EXPORT "__attribute__((visibility(\"default\")))")
        set(IMPORT "")
        set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -fmodule-ts -fvisibility=hidden")
    elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL Clang)
        if (${CMAKE_CXX_PLATFORM_ID} STREQUAL "Windows")
            set(EXPORT "__declspec(dllexport)")
            set(IMPORT "__declspec(dllimport)")
        elseif (${CMAKE_CXX_PLATFORM_ID} STREQUAL "Linux")
            set(EXPORT "__attribute((visibility(\"default\")))")
            set(IMPORT "")
        # elseif(${CMAKE_CXX_PLATFORM_ID} STREQUAL "Apple")
        else ()
            message(FATAL_ERROR "please configure your platform properties manually: ${CMAKE_CXX_PLATFORM_ID}")
        endif ()
    else()
        message(FATAL_ERROR "please configure your compiler properties manually: ${CMAKE_CXX_COMPILER_ID}")
    endif ()
endif ()


if (NOT ${STRICT_MEMORY_MANAGEMENT})
    add_compile_definitions(
        SAFE_LEVEL=2
    )
elseif (${CMAKE_BUILD_TYPE} STREQUAL Debug)
    add_compile_definitions(
        SAFE_LEVEL=0
    )
else ()
    add_compile_definitions(
        SAFE_LEVEL=1
    )
endif ()


set(PNL-Include "${PROJECT_SOURCE_DIR}/include")
file(GLOB pnl-headers
    ${PNL-Include}/*
)
install(FILES ${pnl-headers} DESTINATION "${loc}/include")
add_subdirectory(dependency)
add_subdirectory(src/pnl/ll)



