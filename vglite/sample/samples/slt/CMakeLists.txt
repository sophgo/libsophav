cmake_minimum_required(VERSION 3.16)
project(slt)

set(sample_name test_tde)

set(component libsophon)
include_directories(${VG_lite_path}/inc)
include_directories(${VG_lite_path}/util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/. SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/Cases/. SRC_Case)

link_directories(${VG_lite_path}/LD)

add_executable(${sample_name} ${SRC} ${SRC_Case})
target_link_libraries(${sample_name} vg_lite vg_lite_util m)

INSTALL(TARGETS ${sample_name} RUNTIME DESTINATION ${VG_lite_path}/bin)

install(TARGETS ${sample_name}
    RUNTIME DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})