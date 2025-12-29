#!/bin/bash

# example (run in libsophav root dir)
GCC_V="630"
PLATFORM=soc
DISABLE_BMCV_DOC=on
if [ $# -ge 1 ]; then
    GCC_V=$1
fi

if [ $GCC_V = "930" ]; then
    CMAKE_C_COMPILER=$(which aarch64-linux-gcc)
    CMAKE_CXX_COMPILER=$(which aarch64-linux-g++)
elif [ $GCC_V = "1131" ]; then
    CMAKE_C_COMPILER=$(which aarch64-none-linux-gnu-gcc)
    CMAKE_CXX_COMPILER=$(which aarch64-none-linux-gnu-g++)
elif [ $GCC_V = "pcie" ]; then
    CMAKE_C_COMPILER=$(which gcc)
    CMAKE_CXX_COMPILER=$(which g++)
    PLATFORM=pcie
elif [ $GCC_V = "pcie_arm64" ]; then
    CMAKE_C_COMPILER=$(which aarch64-none-linux-gnu-gcc)
    CMAKE_CXX_COMPILER=$(which aarch64-none-linux-gnu-g++)
    PLATFORM=pcie_arm64
else
    CMAKE_C_COMPILER=$(which aarch64-linux-gnu-gcc)
    CMAKE_CXX_COMPILER=$(which aarch64-linux-gnu-g++)
fi

if ! git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
    echo "当前目录不是 git 仓库"
elif [[ -n $(git status --porcelain bmcv/document/) ]]; then
    echo "BMCV 文档内容有修改，启动编译"
    DISABLE_BMCV_DOC=off
fi

rm -rf buildit install
mkdir buildit
pushd buildit
cmake -DPLATFORM=${PLATFORM} -DSUBTYPE=asic -DCMAKE_INSTALL_PREFIX=../install -DDEBUG=on \
    -DCMAKE_C_COMPILER=${CMAKE_C_COMPILER} \
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER} \
    -DDISABLE_BMCV_DOC=${DISABLE_BMCV_DOC} \
    -DCHIP_NAME=bm1688 \
    -DCMAKE_BUILD_TYPE=Release \
    ..

cmake --build . --target all -- -j`nproc`
cmake --build . --target install
popd
# tar -zcvf sophon_media-v1.0.0.tgz -C install/ .