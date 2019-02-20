
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

# CapDLToolCFileGen(program_1_cspec program_1_cspec.c "${CMAKE_CURRENT_BINARY_DIR}/spec.cdl"
    # "${CAPDL_TOOL_BINARY}"
    # MAX_IRQS ${CapDLLoaderMaxIRQs}
    # DEPENDS ${program_1} install_capdl_tool "${CAPDL_TOOL_BINARY}"
    # )

# BuildCapDLApplication2(
        # C_SPEC "program_1_cspec.c"
        # ELF ${elf_files}
        # DEPENDS ${elf_targets} program_1_cspec
        # OUTPUT "program_1_component"
    # )

# cdl_ld("${CMAKE_CURRENT_BINARY_DIR}/spec2.cdl" capdl_spec2
    # MANIFESTS ${CMAKE_CURRENT_SOURCE_DIR}/manifest2.py
    # ELF program_1_component
    # DEPENDS program_1_component)

# DeclareRootserver("program_1_component")
# DeclareCDLRootImage("${CMAKE_CURRENT_BINARY_DIR}/spec2.cdl" capdl_spec2 ELF program_1_component ELF_DEPENDS program_1_component)

DeclareCDLRootImage("${CMAKE_CURRENT_BINARY_DIR}/spec.cdl" capdl_spec ELF ${elf_files} ELF_DEPENDS ${elf_targets})
