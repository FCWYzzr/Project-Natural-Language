project(NLVMW-Win32)


add_executable(nlvm
    nlvm.cpp
    nlvm.rc
    ${nlvm-common-src}
    ${PNL-Lower-Level-interface}
)
target_include_directories(nlvm PUBLIC
    ${PNL-Include}
)
target_link_libraries(nlvm
    nl-ll
)
target_compile_definitions(nlvm PUBLIC
    "PNL_LIB_PREFIX=${IMPORT}"
)

add_executable(nlvmw
    nlvmw.cpp
    nlvmw.rc
    ${nlvm-common-src}
    ${PNL-Lower-Level-interface}
)
target_include_directories(nlvmw PUBLIC
    ${PNL-Include}
)
target_link_libraries(nlvmw
    nl-ll
)
target_compile_definitions(nlvmw PUBLIC
    "PNL_LIB_PREFIX=${IMPORT}"
)
target_link_options(nlvmw PUBLIC
    /ENTRY:wWinMainCRTStartup
)


install(TARGETS nlvm DESTINATION "${loc}/bin")
install(TARGETS nlvmw DESTINATION "${loc}/bin")