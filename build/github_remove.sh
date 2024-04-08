#!/bin/bash

source build/build_cmake.sh
rm -rf ./bmcv/src/cmodel
sed -i "/github_delete/d" ./bmcv/CMakeLists.txt

cd vglite
rm -rf bin
rm -rf src
rm -rf util
rm -rf lib
rm -rf inc/vg_lite_context.h inc/vg_lite_ioctl.h inc/vg_lite_kernel.h inc/vg_lite_options.h
find . -type f -name CMakeLists.txt -exec sed -i 's|link_directories(${VG_lite_path}/lib)|link_directories(${VG_lite_path}/LD)|g' {} +
find . -type f -name CMakeLists.txt -exec sed -i 's/aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR}\/imgIndex.c SRC)/set(SRC ${CMAKE_CURRENT_SOURCE_DIR}\/imgIndex.c)/g' {} +
echo "cmake_minimum_required(VERSION 3.16)
project(vglite)
set(component libsophon)
set(VGLite_LIB_NAME vg_lite CACHE INTERNAL \"VGLite library name\")
set(VG_lite_path \${CMAKE_CURRENT_SOURCE_DIR})
add_definitions(-D__LINUX__)
install(DIRECTORY \${CMAKE_CURRENT_SOURCE_DIR}/LD/
    DESTINATION lib
    FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
    COMPONENT \${component}
    FILES_MATCHING
    PATTERN \"*vg_lite.*\"  # 或者按照实际情况匹配文件名
)
add_subdirectory(sample)" > CMakeLists.txt
cd ../tde
find . -type f -name CMakeLists.txt -exec sed -i 's|link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../vglite/lib/)|link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../vglite/LD/)|g' {} +
find . -name CMakeLists.txt -exec sed -i 's/target_link_libraries(${sample_name} ${VGLite_LIB_NAME}-static ${TDE_LIB_NAME}-static bmlib dl m pthread atomic)/target_link_libraries(${sample_name} libvg_lite.a ${TDE_LIB_NAME}-static bmlib dl m pthread atomic)/g' {} +
find . -name CMakeLists.txt -exec sed -i 's/target_link_libraries(${VGLite_LIB_NAME})/ /g' {} +
cd ..