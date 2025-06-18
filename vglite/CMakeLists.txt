cmake_minimum_required(VERSION 3.16)
project(vglite)
set(component libsophon)
set(VGLite_LIB_NAME vg_lite CACHE INTERNAL "VGLite library name")
set(VG_lite_path ${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-D__LINUX__)
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/LD/
    DESTINATION lib
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component}
    FILES_MATCHING
    PATTERN "*vg_lite*"  # open source *.so
)
add_subdirectory(sample)
