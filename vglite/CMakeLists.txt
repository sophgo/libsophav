cmake_minimum_required(VERSION 3.16)
project(vglite)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/. SRC)
# set(SRC src/vg_lite_dump.c src/vg_lite_image.c src/vg_lite.c src/vg_lite_ioctl.c src/vg_lite_matrix.c src/vg_lite_os.c src/vg_lite_path.c src/vg_lite_stroke.c)
add_definitions(-D__LINUX__)

set(VGLite_LIB_NAME vg_lite CACHE INTERNAL "VGLite library name")
set(HEADERS inc/vg_lite.h)
set(component libsophon)

add_library(${VGLite_LIB_NAME} SHARED ${SRC})
add_library(${VGLite_LIB_NAME}-static STATIC ${SRC})
# sync static lib name with dynamic lib name
set_target_properties(${VGLite_LIB_NAME}-static PROPERTIES OUTPUT_NAME ${VGLite_LIB_NAME})

set(LIBRARY_OUTPUT_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/lib)

set(VG_lite_path ${CMAKE_CURRENT_SOURCE_DIR})
# message(STATUS "VG_lite_path: ${VG_lite_path}")

install(TARGETS ${VGLite_LIB_NAME}
    LIBRARY DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

install(TARGETS ${VGLite_LIB_NAME}-static
    DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component}-dev)

install(FILES ${HEADERS}
    DESTINATION include
    COMPONENT ${component}-dev)

add_subdirectory(util)
add_subdirectory(sample)


