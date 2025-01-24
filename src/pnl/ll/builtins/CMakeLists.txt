project(PNL-LL-Builtins C CXX)




file(GLOB_RECURSE nl_core_src
    core/*.cpp
    core/*.def
)
add_library(nlrt_core SHARED
    ${nl_core_src}
    ${PNL-Lower-Level-interface}
)
target_include_directories(nlrt_core PUBLIC
    ${PNL-Include}
)
target_link_libraries(nlrt_core PUBLIC
    nl-ll
)
target_compile_definitions(nlrt_core PUBLIC
    "PNL_LIB_PREFIX=${IMPORT}"
)


file(GLOB_RECURSE nl_io_src
    io/*.cpp
    io/*.def
)


add_library(nlrt_io SHARED
    ${nl_io_src}
    ${PNL-Lower-Level-interface}
)
target_include_directories(nlrt_io PUBLIC
    ${PNL-Include}
)
target_link_libraries(nlrt_io PUBLIC
    nl-ll
)
target_compile_definitions(nlrt_io PUBLIC
    "PNL_LIB_PREFIX=${IMPORT}"
)



file(GLOB_RECURSE nlrt
    *.nlsm
)

install(FILES ${nlrt} DESTINATION "${loc}/bin/rt/")
install(TARGETS nlrt_core RUNTIME DESTINATION "${loc}/bin/rt/")
install(TARGETS nlrt_io RUNTIME DESTINATION "${loc}/bin/rt/")