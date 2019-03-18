
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

CreateComponent(
    TARGET component1
    MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/.manifest.obj"
    C_FILES "client.c" "timer.c")

list(APPEND cpio_files "$<TARGET_FILE:component1>")

# second capdl loader app
CreateComponent(
    TARGET component2
    MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/manifest.py"
    C_FILES "program_1.c")

list(APPEND cpio_files "$<TARGET_FILE:component2>")

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
target_link_libraries(loader_final Configuration muslc sel4 elf cpio sel4platsupport sel4utils sel4muslcsys)
add_dependencies(loader_final component1 component2)

DeclareRootserver(loader_final)
