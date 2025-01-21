project(Dependencies)

set(dependencies
    "antlr4"
    "libiconv"
    "dlfcn"
)


# do not modify things below UNLESS YOU KNOW WHAT YOU ARE DOING

if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    set(link_type "debug")
else ()
    set(link_type "release")
endif ()

set(interface "")
set(runtime_lib "")
message("-------- begin scan dependencies --------")
foreach (lib ${dependencies})
    file(GLOB_RECURSE header
        "${PROJECT_SOURCE_DIR}/${lib}/include/*"
    )
    file(GLOB_RECURSE objects_dynamic
        "${PROJECT_SOURCE_DIR}/${lib}/${link_type}/bin/*.${dll_ext}"
    )
    file(GLOB_RECURSE objects_linker
        "${PROJECT_SOURCE_DIR}/${lib}/${link_type}/bin/*.${linker_ext}"
    )
    file(GLOB_RECURSE objects_static
        "${PROJECT_SOURCE_DIR}/${lib}/${link_type}/lib/*.${lib_ext}"
    )
    message(
        "-----${lib}-----\n"
        "header:    ${header}\n"
        "linkers:   ${objects_linker}\n"
        "statics:   ${objects_static}\n"
        "dynamics:  ${objects_dynamic}"
    )
    list(APPEND interface
        ${header}
    )
    set_property(GLOBAL PROPERTY "${lib}-headers" "${PROJECT_SOURCE_DIR}/${lib}/include")
    set_property(GLOBAL PROPERTY "${lib}" "${objects_linker};${objects_static}")
    list(APPEND runtime_lib ${objects_dynamic})


endforeach ()
message("-------- end scan dependencies --------")

add_compile_definitions(
    DLFCN_WIN32_SHARED
)


set_property(GLOBAL PROPERTY dependencies "${dependencies}")

install(FILES ${interface} DESTINATION "${loc}/include")
install(FILES ${runtime_lib} DESTINATION "${loc}/bin")

