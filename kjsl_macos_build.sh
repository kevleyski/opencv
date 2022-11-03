#!/bin/bash

BUILD=cmake-build-debug
BUILD=build

mkdir -p $BUILD
cd $BUILD || exit 1

cmake -D CMAKE_BUILD_TYPE=DEBUG \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D INSTALL_C_EXAMPLES=ON \
    -D INSTALL_PYTHON_EXAMPLES=ON \
    -D OPENCV_GENERATE_PKGCONFIG=ON \
    -D WITH_FFMPEG=ON \
    -D OPENCV_EXTRA_MODULES_PATH=../opencv_contrib/modules \
    -D BUILD_EXAMPLES=ON ..

make -j$(nproc)
sudo make install
