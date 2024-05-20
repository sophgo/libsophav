cmake_minimum_required(VERSION 3.16)
project(venc)

# add header files path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libbmcv/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/osdrv)

# add library files path
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libbmcv/lib/${PLATFORM})
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/lib/${PLATFORM})
#link_directories(${CMAKE_INSTALL_PREFIX}/lib)
#link_directories(${CMAKE_INSTALL_PREFIX}/soc)

# build venc library
set(VPUENC_LIB_NAME bmvenc CACHE INTERNAL "vpuenc library name")
set(SRCS src/bm_ioctl.c src/bm_vpuenc_interface.c src/bm_vpu_logging.c)
set(HEADERS inc/bm_vpuenc_interface.h inc/bmvpuapi_common.h)
set(component libsophon)

add_library(${VPUENC_LIB_NAME} SHARED ${SRCS})
target_link_libraries(${VPUENC_LIB_NAME} bmlib)

# avoid to compile source file repeated
add_library(${VPUENC_LIB_NAME}-static STATIC $<TARGET_OBJECTS:${VPUENC_LIB_NAME}>)
# sync static lib name with dynamic lib name
set_target_properties(${VPUENC_LIB_NAME}-static PROPERTIES OUTPUT_NAME ${VPUENC_LIB_NAME})
target_link_libraries(${VPUENC_LIB_NAME}-static bmlib)

# build venc test case
set(target bmvpuenc)
add_executable(${target} example/bmvpuenc.c example/queue.c)
target_link_libraries(${target} ${VPUENC_LIB_NAME} dl pthread)
target_link_options(${target} PRIVATE -rdynamic)

install(TARGETS ${target}
    RUNTIME DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

install(TARGETS ${VPUENC_LIB_NAME}
    LIBRARY DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

install(TARGETS ${VPUENC_LIB_NAME}-static
    DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component}-dev)

install(FILES ${HEADERS}
    DESTINATION include
    COMPONENT ${component}-dev)

add_custom_target(venc_all ALL DEPENDS ${VPUENC_LIB_NAME} ${VPUENC_LIB_NAME}-static ${target})
