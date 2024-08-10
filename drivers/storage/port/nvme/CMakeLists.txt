list(APPEND SOURCE
     nvmeStd.c
     nvmeStat.c
     nvmeInit.c
     nvmeSnti.c
     nvmeIo.c
     nvmePwrMgmt.c
     # nvmeWmi.c
     )

add_library(nvme MODULE ${SOURCE} nvme.rc)

target_link_libraries(nvme ${PSEH_LIB})

# remove_definitions(-D_WIN32_WINNT=0x502 -DNTDDI_VERSION)

# add_definitions(-D_WIN32_WINNT=0x0A00)
# add_definitions(-DNTDDI_VERSION=0x0A000005)
set_entrypoint(nvme DriverEntry)
set_module_type(nvme kernelmodedriver)
add_importlibs(nvme storport ntoskrnl hal)
add_cd_file(TARGET nvme DESTINATION reactos/system32/drivers NO_CAB FOR all)
add_driver_inf(nvme nvme.inf)
