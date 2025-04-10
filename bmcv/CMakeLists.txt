cmake_minimum_required(VERSION 3.16)
project(bmcv)
# add_subdirectory(./src/cmodel) #github_delete
# options
if("${CHIP_NAME}" STREQUAL "bm1688")
    set(SRC_DIR bm1688)
else()  # default
    set(SRC_DIR bm1688)
endif()

add_compile_options(-Werror)

# add header files path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libbmcv/include)
if("${PLATFORM}" STREQUAL "pcie" OR "${PLATFORM}" STREQUAL "pcie_arm64")
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/osdrv_pcie)
else()
    include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/osdrv)
endif()
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/${SRC_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libjpeg-turbo/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../jpeg/inc)


# add library files path
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/tpu_kernel_module/)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libbmcv/${LIB_VERSION}/${PLATFORM})
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libjpeg-turbo/lib/${PLATFORM})
link_directories(${CMAKE_CURRENT_BINARY_DIR}/../jpeg)

# build bmcv library
set(BMCV_LIB_NAME bmcv CACHE INTERNAL "bmcv library name")
file(GLOB SRCS src/*.c src/${SRC_DIR}/*.c)
set(HEADERS include/bmcv_api_ext.h include/bmcv_api_ext_c.h include/bmcv_api.h)
set(component libsophon)

add_library(${BMCV_LIB_NAME} SHARED ${SRCS})
target_link_libraries(${BMCV_LIB_NAME} bmlib dl bmjpeg turbojpeg)

# avoid to compile source file repeated
add_library(${BMCV_LIB_NAME}-static STATIC $<TARGET_OBJECTS:${BMCV_LIB_NAME}>)
# sync static lib name with dynamic lib name
set_target_properties(${BMCV_LIB_NAME}-static PROPERTIES OUTPUT_NAME ${BMCV_LIB_NAME})
target_link_libraries(${BMCV_LIB_NAME}-static bmlib dl bmjpeg turbojpeg)

# build bmcv test case
if ("${PCIE_MODE}" STREQUAL "on")
    file(GLOB TEST_SRCS test/test_vpss*.c test/test_cv*.c test/test_faiss*.c test/test_gen*.c)
    add_compile_options(-Wno-unused-result)
else ()
    file(GLOB TEST_SRCS test/*.c)
endif ()
foreach(test_src ${TEST_SRCS})
    # filter case name from filename
    string(LENGTH ${test_src} len)
    math(EXPR len "${len} - 2")
    string(SUBSTRING ${test_src} 0 ${len} target)
    string(FIND ${target} "/" index REVERSE)
    math(EXPR index "${index} + 1")
    string(SUBSTRING ${target} ${index} -1 target)
    add_executable(${target} ${test_src})
    target_link_libraries(${target} ${BMCV_LIB_NAME} pthread dl m cmodel)
    install(TARGETS ${target}
    RUNTIME DESTINATION bin
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})
    list(APPEND TEST_CASES ${target})
endforeach()

# build bmcv daily_test case
if (NOT "${PCIE_MODE}" STREQUAL "on")
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/daily_test)
file(GLOB DAILY_TEST_SRCS test/daily_test/*.c)
foreach(test_src ${DAILY_TEST_SRCS})
    # filter case name from filename
    string(LENGTH ${test_src} len)
    math(EXPR len "${len} - 2")
    string(SUBSTRING ${test_src} 0 ${len} target)
    string(FIND ${target} "/" index REVERSE)
    math(EXPR index "${index} + 1")
    string(SUBSTRING ${target} ${index} -1 target)
    add_executable(${target} ${test_src})
    target_link_libraries(${target} ${BMCV_LIB_NAME} pthread dl m)
    set_target_properties(${target} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/daily_test)
    install(TARGETS ${target}
    RUNTIME DESTINATION bin/daily_test
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})
    list(APPEND TEST_CASES ${target})
endforeach()
endif ()

# build bmcv doc
if(NOT DEFINED DISABLE_BMCV_DOC)
    set(DISABLE_BMCV_DOC "on")
endif()
if(NOT DISABLE_BMCV_DOC)
    set(BMCV_DOC ${CMAKE_BINARY_DIR}/SOPHON_BMCV_Technical_Reference_Manual.pdf)

    add_custom_command(
        OUTPUT ${BMCV_DOC}
        COMMAND make clean
        COMMAND make LC_ALL=en_US.UTF-8 pdf
        COMMAND mv ${CMAKE_CURRENT_SOURCE_DIR}/document/bmcv_demo/build/SOPHON_BMCV_Technical_Reference_Manual.pdf ${CMAKE_BINARY_DIR}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/document/bmcv_demo)
else ()
    set(BMCV_DOC "")
endif ()

install(TARGETS ${BMCV_LIB_NAME}
    LIBRARY DESTINATION lib
    PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT ${component})

#string(FIND "${CMAKE_C_COMPILER}" "aarch64-linux-gnu-gcc" COMPILER_FOUND)

install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/libbmcv/${LIB_VERSION}/${PLATFORM}/libcmodel.so
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

install(FILES ${BMCV_DOC}
    DESTINATION bin/res
    COMPONENT ${component}-dev)

add_custom_target(bmcv_all ALL DEPENDS ${BMCV_LIB_NAME} ${BMCV_LIB_NAME}-static ${TEST_CASES} ${BMCV_DOC})
