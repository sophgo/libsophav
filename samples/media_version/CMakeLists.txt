cmake_minimum_required(VERSION 3.16)
project(media_version)

# add header files path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/osdrv)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libbmcv/include)


if("${PLATFORM}" STREQUAL "pcie" OR "${PLATFORM}" STREQUAL "pcie_arm64")
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libbmcv/lib/${PLATFORM})
elseif("${PLATFORM}" STREQUAL "soc")
    if("${GCC_VERSION}" STREQUAL "930")
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libbmcv/lib930/${PLATFORM})
    elseif("${GCC_VERSION}" STREQUAL "1131")
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libbmcv/lib1131/${PLATFORM})
    else()
        link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libbmcv/lib/${PLATFORM})
    endif()
endif()
#jpeg
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../jpeg/3rdparty/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../jpeg/inc)
link_directories(${CMAKE_BINARY_DIR}/jpeg)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libjpeg-turbo/lib/${PLATFORM})
#video dec
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../video/dec/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../video/dec/example)
link_directories(${CMAKE_BINARY_DIR}/video/dec)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../video/dec/helper)

#video enc
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../video/enc/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../video/enc/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../video/enc/example)
link_directories(${CMAKE_BINARY_DIR}/video/enc)

#bmcv
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../bmcv/include)
link_directories(${CMAKE_BINARY_DIR}/bmcv)

set(VPU_LIB  bmvd bmvenc)
set(BMCV_LIB bmcv)

set(target media_version)
add_executable(${target} media_version.c)
target_link_libraries(${target} ${VPU_LIB} ${BMCV_LIB} dl pthread)