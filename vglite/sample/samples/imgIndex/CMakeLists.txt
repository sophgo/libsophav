cmake_minimum_required(VERSION 3.16)
project(imgIndex)

set(sample_name imgIndex)
#message(STATUS "VG_lite_path: ${VG_lite_path}")

include_directories(${VG_lite_path}/inc)
include_directories(${VG_lite_path}/util)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/imgIndex.c SRC)

link_directories(${VG_lite_path}/lib)

add_executable(${sample_name} ${SRC})
target_link_libraries(${sample_name} vg_lite vg_lite_util m)

INSTALL(TARGETS ${sample_name} RUNTIME DESTINATION ${VG_lite_path}/bin)

