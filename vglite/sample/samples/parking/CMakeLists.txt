cmake_minimum_required(VERSION 3.16)
project(parking)

set(sample_name parking)

include_directories(${VG_lite_path}/inc)
include_directories(${VG_lite_path}/util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Resource_6/6_1)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Resource_6/6_3)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Resource_6/6_4)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Resource_6/6_5)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/Resource_6/6_6)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/. SRC)

link_directories(${VG_lite_path}/LD)

add_executable(${sample_name} ${SRC})
target_link_libraries(${sample_name} vg_lite vg_lite_util m)

INSTALL(TARGETS ${sample_name} RUNTIME DESTINATION ${VG_lite_path}/bin)

