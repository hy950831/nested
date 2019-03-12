
#
# Copyright 2018, Data61
# Commonwealth Scientific and Industrial Research Organisation (CSIRO)
# ABN 41 687 119 230.
#
# This software may be distributed and modified according to the terms of
# the BSD 2-Clause license. Note that NO WARRANTY is provided.
# See "LICENSE_BSD2.txt" for details.
#
# @TAG(DATA61_BSD)
#
ImportCapDL()
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -u __vsyscall_ptr")
include(helpers.cmake)


cdl_pp(${CMAKE_CURRENT_SOURCE_DIR}/.manifest.obj cdl_pp_target
    ELF "client"
    CFILE "${CMAKE_CURRENT_BINARY_DIR}/cspace_client.c"

    ELF "timer"
    CFILE "${CMAKE_CURRENT_BINARY_DIR}/cspace_timer.c"
)

add_executable(client EXCLUDE_FROM_ALL client.c cspace_client.c)
add_dependencies(client cdl_pp_target)
target_link_libraries(client sel4tutorials)

list(APPEND elf_files "$<TARGET_FILE:client>")
list(APPEND elf_targets "client")

add_executable(timer EXCLUDE_FROM_ALL timer.c cspace_timer.c)
add_dependencies(timer cdl_pp_target)
target_link_libraries(timer sel4tutorials)

list(APPEND elf_files "$<TARGET_FILE:timer>")
list(APPEND elf_targets "timer")

cdl_ld("${CMAKE_CURRENT_BINARY_DIR}/spec.cdl" capdl_spec
    MANIFESTS ${CMAKE_CURRENT_SOURCE_DIR}/.manifest.obj
    ELF ${elf_files}
    DEPENDS ${elf_targets})

CapDLToolCFileGen(${capdl_spec}_cspec ${capdl_spec}_cspec.c "${CMAKE_CURRENT_BINARY_DIR}/spec.cdl" "${CAPDL_TOOL_BINARY}"
    MAX_IRQS ${CapDLLoaderMaxIRQs}
    DEPENDS ${capdl_spec} install_capdl_tool "${CAPDL_TOOL_BINARY}")

BuildCapDLApplication(
        C_SPEC "${capdl_spec}_cspec.c"
        ELF ${elf_files}
        DEPENDS ${elf_targets} ${capdl_spec}_cspec
        OUTPUT "capdl-app"
    )

SetSeL4StartComponent(capdl-app)
list(APPEND cpio_files "$<TARGET_FILE:capdl-app>")

add_executable(loader EXCLUDE_FROM_ALL loader.c)
target_link_libraries(loader sel4tutorials)

MakeCPIO(archive_final.o ${cpio_files}
    CPIO_SYMBOL _component_cpio
)

add_executable(loader_final EXCLUDE_FROM_ALL
    loader.c
    archive_final.o
    )

add_dependencies(loader_final ${elf_targets} capdl-app)
target_link_libraries(loader_final Configuration muslc sel4 elf cpio sel4platsupport sel4utils sel4muslcsys)
target_include_directories(loader_final PRIVATE $<TARGET_PROPERTY:capdl_app_properties,INCLUDE_DIRS>)

DeclareRootserver(loader_final)

# DeclareCDLRootImage("${CMAKE_CURRENT_BINARY_DIR}/spec.cdl" capdl_spec ELF ${elf_files} ELF_DEPENDS ${elf_targets})
