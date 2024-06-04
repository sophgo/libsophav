cmake_minimum_required(VERSION 3.16)
project(bmcv)
# options
if("${CHIP_NAME}" STREQUAL "bm1688")
    set(SRC_DIR bm1688)
else()  # default
    set(SRC_DIR bm1688)
endif()

add_compile_options(-Werror)

# add header files path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libbmcv/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/osdrv)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/${SRC_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libjpeg-turbo/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../jpeg/inc)

# add library files path
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/tpu_kernel_module/)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libbmcv/lib/${PLATFORM})
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libjpeg-turbo/lib/${PLATFORM})
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../jpeg)

# build bmcv library
set(BMCV_LIB_NAME bmcv CACHE INTERNAL "bmcv library name")
file(GLOB SRCS src/*.c src/${SRC_DIR}/*.c)
set(HEADERS include/bmcv_api_ext.h include/bmcv_api_ext_c.h)
set(component libsophon)

add_library(${BMCV_LIB_NAME} SHARED ${SRCS})
target_link_libraries(${BMCV_LIB_NAME} bmlib dl bmjpeg turbojpeg cmodel)

# avoid to compile source file repeated
add_library(${BMCV_LIB_NAME}-static STATIC $<TARGET_OBJECTS:${BMCV_LIB_NAME}>)
# sync static lib name with dynamic lib name
set_target_properties(${BMCV_LIB_NAME}-static PROPERTIES OUTPUT_NAME ${BMCV_LIB_NAME})
target_link_libraries(${BMCV_LIB_NAME}-static bmlib dl bmjpeg turbojpeg cmodel)

# build bmcv test case
file(GLOB TEST_SRCS test/*.c)
foreach(test_src ${TEST_SRCS})
    # filter case name from filename
    string(LENGTH ${test_src} len)
    math(EXPR len "${len} - 2")
    string(SUBSTRING ${test_src} 0 ${len} target)
    string(FIND ${target} "/" index REVERSE)
    math(EXPR index "${index} + 1")
    string(SUBSTRING ${target} ${index} -1 target)
    add_executable(${target} ${test_src})
    target_link_libraries(${target} ${BMCV_LIB_NAME} pthread dl m)
    install(TARGETS ${target}
    RUNTIME DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})
    list(APPEND TEST_CASES ${target})
endforeach()

install(TARGETS ${BMCV_LIB_NAME}
    LIBRARY DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libbmcv/lib/${PLATFORM}/libcmodel.so
    DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/tpu_kernel_module/libbm1688_kernel_module.so
    DESTINATION lib/tpu_module/
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

install(TARGETS ${BMCV_LIB_NAME}-static
    DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component}-dev)

install(FILES ${HEADERS}
    DESTINATION include
    COMPONENT ${component}-dev)

install(FILES test/test_bmcv_all.sh
    DESTINATION bin
    COMPONENT ${component}-dev)

install(DIRECTORY test/res
    DESTINATION bin
    COMPONENT ${component}-dev)

add_custom_target(bmcv_all ALL DEPENDS ${BMCV_LIB_NAME} ${BMCV_LIB_NAME}-static ${TEST_CASES})
