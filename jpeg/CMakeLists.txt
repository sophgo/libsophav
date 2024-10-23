cmake_minimum_required(VERSION 3.16)
project(jpeg)

if("${SUBTYPE}" STREQUAL "fpga")
    add_definitions(-DBOARD_FPGA=1)
endif()

# add header files path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libbmcv/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/osdrv)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/inc)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/example)

# add library files path
string(FIND "${CMAKE_C_COMPILER}" "aarch64-linux-gnu-gcc" COMPILER_FOUND)
if (COMPILER_FOUND GREATER -1)
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libbmcv/lib/${PLATFORM})
else ()
    link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libbmcv/lib930/${PLATFORM})
endif ()

set(JPEG_LIB_NAME bmjpeg CACHE INTERNAL "jpeg library name")
set(SRCS src/bm_ioctl.c src/bm_jpeg_interface.c src/bm_jpeg_logging.c)
set(HEADERS inc/bm_jpeg_interface.h inc/bm_jpeg_internal.h)
set(component libsophon)

add_library(${JPEG_LIB_NAME} SHARED ${SRCS})
target_link_libraries(${JPEG_LIB_NAME} bmlib)

# avoid to compile source file repeated
add_library(${JPEG_LIB_NAME}-static STATIC $<TARGET_OBJECTS:${JPEG_LIB_NAME}>)
# sync static lib name with dynamic lib name
set_target_properties(${JPEG_LIB_NAME}-static PROPERTIES OUTPUT_NAME ${JPEG_LIB_NAME})
target_link_libraries(${JPEG_LIB_NAME}-static bmlib)

# build jpeg test case
set(target bmjpegdec)
add_executable(${target} example/helper/md5.c example/jpeg_dec_common.c example/jpeg_dec_test.c)
target_link_libraries(${target} ${JPEG_LIB_NAME} dl pthread)
install(TARGETS ${target}
    RUNTIME DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

set(target bmjpegenc)
add_executable(${target} example/helper/md5.c example/jpeg_enc_common.c example/jpeg_enc_test.c)
target_link_libraries(${target} ${JPEG_LIB_NAME} dl pthread)
install(TARGETS ${target}
    RUNTIME DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

set(target bmjpegmulti)
add_executable(${target} example/helper/md5.c example/bmjpurun.c example/jpeg_multi_test.c)
target_link_libraries(${target} ${JPEG_LIB_NAME} dl pthread)
install(TARGETS ${target}
    RUNTIME DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

install(TARGETS ${JPEG_LIB_NAME}
    LIBRARY DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

install(TARGETS ${JPEG_LIB_NAME}-static
    DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component}-dev)

install(FILES ${HEADERS}
    DESTINATION include
    COMPONENT ${component}-dev)

add_custom_target(jpeg_all ALL DEPENDS ${JPEG_LIB_NAME} ${JPEG_LIB_NAME}-static bmjpegdec bmjpegenc bmjpegmulti)
