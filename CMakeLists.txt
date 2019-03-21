
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
    MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/component1/.manifest.obj"
    C_FILES "component1/client.c" "component1/timer.c")
list(APPEND components "$<TARGET_FILE:component1>")

# second capdl loader app
CreateComponent(
    TARGET component2
    MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/component2/manifest.py"
    C_FILES "component2/program_1.c")
list(APPEND components "$<TARGET_FILE:component2>")

# second capdl loader app
CreateComponent(
    TARGET component3
    MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/component3/manifest.py"
    C_FILES "component3/program_2.c" "component3/program_3.c" )
list(APPEND components "$<TARGET_FILE:component3>")

# the combined loader
add_executable(loader EXCLUDE_FROM_ALL loader.c)
target_link_libraries(loader sel4tutorials)

MakeCPIO(archive_final.o "${components}"
    CPIO_SYMBOL _component_cpio
    DEPENDS ${components}
    )

add_executable(loader_final EXCLUDE_FROM_ALL
    loader.c
    archive_final.o
    )
target_link_libraries(loader_final Configuration muslc sel4 elf cpio sel4platsupport sel4utils sel4muslcsys)
add_dependencies(loader_final  component2 component3)

DeclareRootserver(loader_final)
