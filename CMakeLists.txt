
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
include(helpers.cmake)
ImportCapDL()
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -u __vsyscall_ptr")

add_subdirectory(component1)
add_subdirectory(component2)
add_subdirectory(component3)

list(APPEND components "$<TARGET_FILE:component1>")
list(APPEND components "$<TARGET_FILE:component2>")
list(APPEND components "$<TARGET_FILE:component3>")

MakeCPIO(archive_final.o "${components}"
    CPIO_SYMBOL _component_cpio
    DEPENDS ${components}
    )

add_executable(loader_final EXCLUDE_FROM_ALL
    loader.c
    archive_final.o
    )

target_link_libraries(loader_final Configuration muslc sel4 elf cpio sel4platsupport sel4utils sel4muslcsys)
add_dependencies(loader_final component1 component2 component3)

DeclareRootserver(loader_final)
