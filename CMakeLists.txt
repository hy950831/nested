
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

# first capdl loader app
cdl_pp(${CMAKE_CURRENT_SOURCE_DIR}/.manifest.obj cdl_pp_target1
    ELF "client"
    CFILE "${CMAKE_CURRENT_BINARY_DIR}/cspace_client.c"

    ELF "timer"
    CFILE "${CMAKE_CURRENT_BINARY_DIR}/cspace_timer.c"
)

add_executable(client EXCLUDE_FROM_ALL client.c cspace_client.c)
add_dependencies(client cdl_pp_target1)
target_link_libraries(client sel4tutorials)

list(APPEND elf_files1 "$<TARGET_FILE:client>")
list(APPEND elf_targets1 "client")

add_executable(timer EXCLUDE_FROM_ALL timer.c cspace_timer.c)
add_dependencies(timer cdl_pp_target1)
target_link_libraries(timer sel4tutorials)

list(APPEND elf_files1 "$<TARGET_FILE:timer>")
list(APPEND elf_targets1 "timer")

cdl_ld("${CMAKE_CURRENT_BINARY_DIR}/spec.cdl" capdl_spec
    MANIFESTS ${CMAKE_CURRENT_SOURCE_DIR}/.manifest.obj
    ELF ${elf_files1}
    DEPENDS ${elf_targets1})

CapDLToolCFileGen(${capdl_spec}_cspec ${capdl_spec}_cspec.c "${CMAKE_CURRENT_BINARY_DIR}/spec.cdl" "${CAPDL_TOOL_BINARY}"
    MAX_IRQS ${CapDLLoaderMaxIRQs}
    DEPENDS ${capdl_spec} install_capdl_tool "${CAPDL_TOOL_BINARY}")

BuildCapDLApplication(
        C_SPEC "${capdl_spec}_cspec.c"
        ELF ${elf_files1}
        DEPENDS ${elf_targets1} ${capdl_spec}_cspec
        OUTPUT "capdl-app1"
    )

SetSeL4StartComponent(capdl-app1)
list(APPEND cpio_files "$<TARGET_FILE:capdl-app1>")

# second capdl loader app
cdl_pp(${CMAKE_CURRENT_SOURCE_DIR}/manifest.py cdl_pp_target2
    ELF "program_1"
    CFILE "${CMAKE_CURRENT_BINARY_DIR}/cspace_program_1.c"
)

add_executable(program_1 EXCLUDE_FROM_ALL program_1.c cspace_program_1.c)
add_dependencies(program_1 cdl_pp_target2)
target_link_libraries(program_1 sel4tutorials)

list(APPEND elf_files2 "$<TARGET_FILE:program_1>")
list(APPEND elf_targets2 "program_1")

cdl_ld("${CMAKE_CURRENT_BINARY_DIR}/spec2.cdl" capdl_spec2
    MANIFESTS ${CMAKE_CURRENT_SOURCE_DIR}/manifest.py
    ELF ${elf_files2}
    DEPENDS ${elf_targets2})

CapDLToolCFileGen(${capdl_spec2}_cspec2 ${capdl_spec2}_cspec2.c "${CMAKE_CURRENT_BINARY_DIR}/spec2.cdl"
    "${CAPDL_TOOL_BINARY}"
    MAX_IRQS ${CapDLLoaderMaxIRQs}
    DEPENDS program_1 install_capdl_tool "${CAPDL_TOOL_BINARY}"
    )

BuildCapDLApplication(
        C_SPEC "${capdl_spec2}_cspec2.c"
        ELF ${elf_files2}
        DEPENDS ${elf_targets2} ${capdl_spec2}_cspec2
        OUTPUT "capdl-app2"
    )
SetSeL4StartComponent(capdl-app2)

list(APPEND cpio_files "$<TARGET_FILE:capdl-app2>")


# the combined loader

add_executable(loader EXCLUDE_FROM_ALL loader.c)
target_link_libraries(loader sel4tutorials)

MakeCPIO(archive_final.o "${cpio_files}"
    CPIO_SYMBOL _component_cpio
)

add_executable(loader_final EXCLUDE_FROM_ALL
    loader.c
    archive_final.o
    )

add_dependencies(loader_final ${elf_targets1} ${elf_targets2} capdl-app1 capdl-app2)

target_link_libraries(loader_final Configuration muslc sel4 elf cpio sel4platsupport sel4utils sel4muslcsys)

target_include_directories(loader_final PRIVATE $<TARGET_PROPERTY:capdl_app_properties,INCLUDE_DIRS>)

DeclareRootserver(loader_final)
