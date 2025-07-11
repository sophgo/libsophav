cmake_minimum_required(VERSION 3.16)
project(vdec)

# add header files path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
if("${PLATFORM}" STREQUAL "pcie" OR "${PLATFORM}" STREQUAL "pcie_arm64")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/osdrv_pcie)
else()
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/osdrv)
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libbmcv/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/example)

#string(FIND "${CMAKE_C_COMPILER}" "aarch64-linux-gnu-gcc" COMPILER_FOUND)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libbmcv/${LIB_VERSION}/${PLATFORM})
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/helper)

set(VPU_DEC_LIB_NAME bmvd CACHE INTERNAL "vdec library name")
set(SRCS src/bm_ioctl.c src/bm_vpudec_interface.c)
set(HEADERS inc/bm_vpudec_interface.h)
set(component libsophon)

add_library(${VPU_DEC_LIB_NAME} SHARED ${SRCS})
target_link_libraries(${VPU_DEC_LIB_NAME} bmlib ${CMAKE_CURRENT_SOURCE_DIR}/helper/libsys.a)

# avoid to compile source file repeated
add_library(${VPU_DEC_LIB_NAME}-static STATIC $<TARGET_OBJECTS:${VPU_DEC_LIB_NAME}>)
# sync static lib name with dynamic lib name
set_target_properties(${VPU_DEC_LIB_NAME}-static PROPERTIES OUTPUT_NAME ${VPU_DEC_LIB_NAME})
target_link_libraries(${VPU_DEC_LIB_NAME}-static bmlib ${CMAKE_CURRENT_SOURCE_DIR}/helper/libsys.a)

# build bm_test
set(target bm_test)
add_executable(${target} example/bm_test.c example/osal.c example/md5.c)
target_link_libraries(${target} ${VPU_DEC_LIB_NAME} dl pthread)

install(TARGETS ${target}
    RUNTIME DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

install(TARGETS ${VPU_DEC_LIB_NAME}
    LIBRARY DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

install(TARGETS ${VPU_DEC_LIB_NAME}-static
    DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component}-dev)

install(FILES ${HEADERS}
    DESTINATION include
    COMPONENT ${component}-dev)

add_custom_target(vdec_all ALL DEPENDS ${VPU_DEC_LIB_NAME} ${VPU_DEC_LIB_NAME}-static ${target})
