#!/bin/bash

# example (run in sophon_media root dir)
CMAKE_C_COMPILER=$(which aarch64-linux-gnu-gcc)
CMAKE_CXX_COMPILER=$(which aarch64-linux-gnu-g++)
rm -rf buildit install
mkdir buildit
pushd buildit
cmake -DPLATFORM=soc -DSUBTYPE=asic -DCMAKE_INSTALL_PREFIX=../install -DDEBUG=off -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} -DCHIP_NAME=bm1688 -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --target all -- -j`nproc`
cmake --build . --target install
popd
# tar -zcvf sophon_media-v1.0.0.tgz -C install/ .