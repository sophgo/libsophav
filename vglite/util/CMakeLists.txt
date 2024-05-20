cmake_minimum_required(VERSION 3.16)
project(util)

add_definitions(-DGPU_CHIP_ID_GCNanoUltraV=1)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libpng-1.6.1)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/zlib-1.2.7)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/)

set(HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/vg_lite_util.h)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/. SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/libpng-1.6.1/. SRC_png)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/zlib-1.2.7/. SRC_zlib)

link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../lib/)
target_link_libraries(vg_lite)

add_library(vg_lite_util SHARED ${SRC} ${SRC_png} ${SRC_zlib})

add_library(vg_lite_util-static STATIC ${SRC} ${SRC_png} ${SRC_zlib})
# sync static lib name with dynamic lib name
set_target_properties(vg_lite_util-static PROPERTIES OUTPUT_NAME vg_lite_util)

set(LIBRARY_OUTPUT_PATH  ${CMAKE_CURRENT_SOURCE_DIR}/../lib)

install(TARGETS vg_lite_util
    LIBRARY DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

install(TARGETS vg_lite_util-static
    DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component}-dev)

install(FILES ${HEADERS}
    DESTINATION include
    COMPONENT ${component}-dev)
