#!/bin/bash

BUILD=cmake-build-debug

mkdir -p $BUILD
cd $BUILD || exit 1

export LDFLAGS="-L/usr/local/opt/libffi/lib"
export CPPFLAGS="-I/usr/local/opt/libffi/include"

PKG_CONFIG_PATH="/usr/local/opt/libffi/lib/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig:$PKG_CONFIG_PATH" cmake -D CMAKE_BUILD_TYPE=DEBUG \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D INSTALL_C_EXAMPLES=ON \
    -D INSTALL_PYTHON_EXAMPLES=ON \
    -D OPENCV_GENERATE_PKGCONFIG=ON \
    -D WITH_FFMPEG=ON \
    -D WITH_OPENCL=ON \
    -D OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
    -D BUILD_EXAMPLES=ON ..

make -j$(nproc)
sudo make install
