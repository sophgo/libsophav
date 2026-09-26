cmake_minimum_required(VERSION 3.16)
project(ft_255)

set(sample_name ft_255)

include_directories(${VG_lite_path}/inc)
include_directories(${VG_lite_path}/util)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/. SRC)
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}/src/SFT_Cases/. SRC_Case)

link_directories(${VG_lite_path}/lib)

add_executable(${sample_name} ${SRC} ${SRC_Case})
target_link_libraries(${sample_name} vg_lite vg_lite_util m)

INSTALL(TARGETS ${sample_name} RUNTIME DESTINATION ${VG_lite_path}/bin)
