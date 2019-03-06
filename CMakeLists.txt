
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


cdl_pp(${CMAKE_CURRENT_SOURCE_DIR}/manifest.py cdl_pp_target
    ELF "program_1"
    CFILE "${CMAKE_CURRENT_BINARY_DIR}/cspace_program_1.c"
)

add_executable(program_1 EXCLUDE_FROM_ALL program_1.c cspace_program_1.c)
add_dependencies(program_1 cdl_pp_target)
target_link_libraries(program_1 sel4tutorials)

list(APPEND elf_files "$<TARGET_FILE:program_1>")
list(APPEND elf_targets "program_1")

cdl_ld("${CMAKE_CURRENT_BINARY_DIR}/spec.cdl" capdl_spec
    MANIFESTS ${CMAKE_CURRENT_SOURCE_DIR}/manifest.py
    ELF ${elf_files}
    DEPENDS ${elf_targets})

add_executable(loader EXCLUDE_FROM_ALL loader.c)
target_link_libraries(loader sel4tutorials)

CapDLToolCFileGen(program_1_cspec program_1_cspec.c "${CMAKE_CURRENT_BINARY_DIR}/spec.cdl"
    "${CAPDL_TOOL_BINARY}"
    MAX_IRQS ${CapDLLoaderMaxIRQs}
    DEPENDS ${program_1} install_capdl_tool "${CAPDL_TOOL_BINARY}"
    )

BuildCapDLApplication(
        C_SPEC "program_1_cspec.c"
        ELF ${elf_files}
        DEPENDS ${elf_targets} program_1_cspec
        OUTPUT "capdl-app"
    )

SetSeL4StartComponent(capdl-app)

list(APPEND cpio_files "$<TARGET_FILE:capdl-app>")
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
