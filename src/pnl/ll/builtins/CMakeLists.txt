project(PNL-LL-Builtins C CXX)

get_property(loc GLOBAL PROPERTY loc)



file(GLOB_RECURSE nl_core_src
    anonymous_core/*
)
add_library(NLCore SHARED
    ${nl_core_src}
    ${PNL-Lower-Level-interface}
)
target_link_libraries(NLCore PUBLIC
    NL_LL
)
target_compile_definitions(NLCore PUBLIC
    PNL_LIB_PREFIX=__declspec\(dllimport\)
)


file(GLOB_RECURSE nl_io_src
    io/*
)


add_library(NL_IO SHARED
    ${nl_io_src}
    ${PNL-Lower-Level-interface}
)
target_link_libraries(NL_IO PUBLIC
    NL_LL
)
target_compile_definitions(NL_IO PUBLIC
    PNL_LIB_PREFIX=__declspec\(dllimport\)
)

install(TARGETS NLCore DESTINATION "${loc}/LL/rt/")
install(TARGETS NL_IO DESTINATION "${loc}/LL/rt/")