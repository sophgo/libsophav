cmake_minimum_required(VERSION 3.16)
project(no_share_file)

if("${CHIP_NAME}" STREQUAL "bm1688")
    set(SRC_DIR bm1688)
else()  # default
    set(SRC_DIR bm1688)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/libbmcv/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../jpeg/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../${SRC_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../../3rdparty/osdrv)

file(GLOB SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.c)

set(CMODEL_LIB_NAME cmodel CACHE INTERNAL "cmodel library name")

add_library(${CMODEL_LIB_NAME} SHARED ${SRCS})

string(FIND "${CMAKE_C_COMPILER}" "aarch64-linux-gnu-gcc" COMPILER_FOUND)

if (COMPILER_FOUND GREATER -1)
    set(MY_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/../../../../../libsophav/3rdparty/libbmcv/lib/soc")
else ()
    set(MY_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/../../../../../libsophav/3rdparty/libbmcv/lib930/soc")
endif ()

add_custom_command(TARGET ${CMODEL_LIB_NAME} POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${CMODEL_LIB_NAME}> ${MY_OUTPUT_DIR}
                   COMMENT "Moving dynamic library to ${MY_OUTPUT_DIR}")

add_custom_target(cmodel_all ALL DEPENDS ${CMODEL_LIB_NAME})
