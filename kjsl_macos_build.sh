#!/bin/bash

BUILD=cmake-build-debug
HERE=$PWD

mkdir -p $BUILD
cd $BUILD || exit 1

zlib () {
  cd $HERE/.. || exit 1
  git clone git@github.com:kevleyski/zlib.git
  cd zlib
  ./configure
  make -j8
  sudo make install
}

cd $HERE || exit 1

export LDFLAGS="-L/usr/local/opt/libffi/lib"
export CPPFLAGS="-I/usr/local/opt/libffi/include"

brew install openh264

mkdir -p cmake-build-debug
cd cmake-build-debug || exit 1

PKG_CONFIG_PATH="/usr/local/opt/libffi/lib/pkgconfig:/usr/local/lib/pkgconfig:/usr/lib/pkgconfig:$PKG_CONFIG_PATH" cmake -D CMAKE_BUILD_TYPE=DEBUG \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DINSTALL_C_EXAMPLES=ON \
    -DINSTALL_PYTHON_EXAMPLES=ON \
    -DOPENCV_GENERATE_PKGCONFIG=ON \
    -DWITH_FFMPEG=ON \
    -DFFMPEG_LIBDIR="/usr/local/lib" -DFFMPEG_INCLUDE_DIRS="/usr/local/include" -D OPENCV_FFMPEG_SKIP_BUILD_CHECK=ON \
    -DWITH_OPENCL=ON \
    -DBUILD_ZLIB=ON \
    -DBUILD_OPENEXR=ON \
    -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
    -DBUILD_EXAMPLES=ON ..

make -j$(nproc)
sudo make install
