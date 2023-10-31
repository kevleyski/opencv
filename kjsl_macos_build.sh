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
    -DCMAKE_CXX_STANDARD=11 \
    -DCMAKE_OSX_DEPLOYMENT_TARGET= \
    -DBUILD_JASPER=OFF \
    -DBUILD_JPEG=OFF \
    -DBUILD_OPENEXR=OFF \
    -DBUILD_OPENJPEG=OFF \
    -DBUILD_PERF_TESTS=OFF \
    -DBUILD_PNG=OFF \
    -DBUILD_PROTOBUF=OFF \
    -DBUILD_TBB=OFF \
    -DBUILD_TESTS=OFF \
    -DBUILD_TIFF=OFF \
    -DBUILD_WEBP=OFF \
    -DBUILD_ZLIB=OFF \
    -DBUILD_opencv_hdf=OFF \
    -DBUILD_opencv_java=OFF \
    -DBUILD_opencv_text=ON \
    -DOPENCV_ENABLE_NONFREE=ON \
    -DINSTALL_PYTHON_EXAMPLES=ON \
    -DOPENCV_GENERATE_PKGCONFIG=ON \
    -DWITH_FFMPEG=ON \
    -DFFMPEG_LIBDIR="/usr/local/lib" -DFFMPEG_INCLUDE_DIRS="/usr/local/include" -D OPENCV_FFMPEG_SKIP_BUILD_CHECK=ON \
    -DWITH_OPENCL=ON \
    -DOPENCV_GENERATE_PKGCONFIG=ON \
    -DPROTOBUF_UPDATE_FILES=ON \
    -DWITH_1394=OFF \
    -DWITH_CUDA=OFF \
    -DWITH_EIGEN=ON \
    -DWITH_FFMPEG=ON \
    -DWITH_GPHOTO2=OFF \
    -DWITH_GSTREAMER=OFF \
    -DWITH_JASPER=OFF \
    -DWITH_OPENEXR=ON \
    -DWITH_OPENGL=OFF \
    -DWITH_OPENVINO=ON \
    -DWITH_QT=OFF \
    -DWITH_TBB=ON \
    -DWITH_VTK=ON \
    -DBUILD_opencv_python2=OFF \
    -DBUILD_opencv_python3=ON \
    -DBUILD_ZLIB=ON \
    -DBUILD_OPENEXR=ON \
    -DOPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
    -DBUILD_EXAMPLES=ON ..

make -j$(nproc)
sudo make install
