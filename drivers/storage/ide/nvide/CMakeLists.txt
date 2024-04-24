
list(APPEND SOURCE
    nvide.c
    nvide.h)

add_library(nvide MODULE ${SOURCE} nvide.rc)

set_module_type(nvide kernelmodedriver)
if(MSVC)
    target_link_options(nvide PRIVATE /SECTION:.rsrc,!D)
else()
    add_custom_command(
        TARGET nvide POST_BUILD
        COMMAND native-pefixup --section:.rsrc,!D $<TARGET_FILE:nvide>
        VERBATIM)
endif()
add_pch(nvide nvide.h SOURCE)
add_importlibs(nvide pciidex ntoskrnl)
add_cd_file(TARGET nvide DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_registry_inf(nvide_reg.inf)
