cmake_minimum_required(VERSION 3.16)

# options
if("${PLATFORM}" STREQUAL "soc")
    set(LIB_PATH lib_aarch64)
else()
    set(LIB_PATH lib_musl_riscv64)
endif()

# add header files path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libdrm/include/libdrm)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libdrm/include/libkms)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libdrm/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/common)

# add library files path
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libdrm/${LIB_PATH})

set(DRM_LIB_NAME drm_com CACHE INTERNAL "drm library name")
file(GLOB SRCS common/*.c)
set(component libsophon)

add_library(${DRM_LIB_NAME} SHARED ${SRCS})
target_link_libraries(${DRM_LIB_NAME} -ldrm -lkms -lm)

# avoid to compile source file repeated
add_library(${DRM_LIB_NAME}-static STATIC $<TARGET_OBJECTS:${DRM_LIB_NAME}>)
# sync static lib name with dynamic lib name
set_target_properties(${DRM_LIB_NAME}-static PROPERTIES OUTPUT_NAME ${DRM_LIB_NAME})
target_link_libraries(${DRM_LIB_NAME}-static -ldrm -lkms -lm)

# build drm test case
file(GLOB TEST_SRCS *.c)
foreach(test_src ${TEST_SRCS})
    string(LENGTH ${test_src} len)
    math(EXPR len "${len} - 2")
    string(SUBSTRING ${test_src} 0 ${len} target)
    string(FIND ${target} "/" index REVERSE)
    math(EXPR index "${index} + 1")
    string(SUBSTRING ${target} ${index} -1 target)
    add_executable(${target} ${test_src})
    target_link_libraries(${target} ${DRM_LIB_NAME} -ldrm -lkms -lm)
    install(TARGETS ${target}
    RUNTIME DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})
    list(APPEND TEST_CASES ${target})
endforeach()

add_custom_target(drm_all ALL DEPENDS ${DRM_LIB_NAME} ${DRM_LIB_NAME}-static ${TEST_CASES})

install(TARGETS ${DRM_LIB_NAME}
    LIBRARY DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

install(TARGETS ${DRM_LIB_NAME}-static
    DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component}-dev)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../../3rdparty/libdrm/${LIB_PATH}/
    DESTINATION lib
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component}
    FILES_MATCHING
    PATTERN "*.so*")
