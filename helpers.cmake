
function(CreateCDLRootImage cdl cdl_target)
    cmake_parse_arguments(PARSE_ARGV 2 CDLROOTTASK "" "" "ELF;ELF_DEPENDS")
    if (NOT "${CDLROOTTASK_UNPARSED_ARGUMENTS}" STREQUAL "")
        message(FATAL_ERROR "Unknown arguments to DeclareCDLRootImage")
    endif()

    CapDLToolCFileGen(${cdl_target}_cspec ${cdl_target}_cspec.c ${cdl} "${CAPDL_TOOL_BINARY}"
        MAX_IRQS ${CapDLLoaderMaxIRQs}
        DEPENDS ${cdl_target} install_capdl_tool "${CAPDL_TOOL_BINARY}")

    # Ask the CapDL tool to generate an image with our given copied/mangled instances
    BuildCapDLApplication(
        C_SPEC "${cdl_target}_cspec.c"
        ELF ${CDLROOTTASK_ELF}
        DEPENDS ${CDLROOTTASK_ELF_DEPENDS} ${cdl_target}_cspec
        OUTPUT "capdl-loader"
    )

    DeclareRootserver("capdl-loader")
endfunction()

function(SetSeL4StartComponent target)
    set_property(TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " -u _sel4_start_component -e _sel4_start_component ")
endfunction(SetSeL4StartComponent)
