#!/bin/bash

sudo apt install build-essential cmake git pkg-config libgtk-3-dev \
    libavcodec-dev libavformat-dev libswscale-dev libv4l-dev \
    libxvidcore-dev libx264-dev libjpeg-dev libpng-dev libtiff-dev \
    gfortran openexr libatlas-base-dev python3-dev python3-numpy \
    libtbb2 libtbb-dev

mkdir -p cmake-build-release
cd cmake-build-release || exit 1

export OPENCV_VERSION=4.10.0 \

export CUDA_ARCH_BIN=70
export CUDA_ARCH_PTX=70

export FFmpegPath=/home/ubuntu/kjsl/ffmpeg
export CUDA_TOOLKIT_ROOT_DIR="/usr/local/cuda"
export CC=/usr/bin/gcc-12
export CPP=/usr/bin/g++-12

cmake -D CMAKE_BUILD_TYPE=RELEASE \
    -D CMAKE_INSTALL_PREFIX=/usr/local \
    -D OPENCV_GENERATE_PKGCONFIG=ON \
    -D WITH_FFMPEG=ON \
        -DOPENCV_GENERATE_PKGCONFIG=ON \
        -DBUILD_opencv_world=OFF \
        -DBUILD_opencv_gapi=OFF \
        -DCMAKE_C_COMPILER=gcc-12 \
        -DCMAKE_CXX_COMPILER=g++-12 \
        -DCUDA_ARCH_BIN="${CUDA_ARCH_BIN}" -DCUDA_ARCH_PTX="${CUDA_ARCH_PTX}" \
        -DWITH_FFMPEG=ON \
        -DBUILD_DOCS=OFF \
        -DWITH_NVCUVID=ON \
        -DWITH_CUDA=ON \
        -DCUDA_TOOLKIT_ROOT_DIR=$CUDA_TOOLKIT_ROOT_DIR \
        -DWITH_CUDNN=ON \
        -DWITH_CUBLAS=ON \
        -DENABLE_FAST_MATH=ON \
        -DCUDA_FAST_MATH=ON \
        -DOPENCV_DNN_CUDA=ON \
        -DBUILD_DOCS=OFF \
        -D WITH_PROTOBUF=OFF \
        -DBUILD_TESTS=OFF \
        -DBUILD_PERF_TESTS=OFF \
    -D WITH_TBB=ON \
    -D OPENCV_EXTRA_MODULES_PATH=../../opencv_contrib/modules \
    -D BUILD_EXAMPLES=ON ..

make -j$(nproc)
sudo make install
