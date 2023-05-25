FROM ubuntu:22.04

 

ARG DEBIAN_FRONTEND=noninteractive 


WORKDIR /root

RUN apt-get update -y && apt-get install -y \
    git \
    cmake \
    capnproto \
    zlib1g-dev \
    g++ \
    build-essential \
    python3-dev \
    python3-pip \
    pkg-config \
    libzip-dev \
    software-properties-common \
    g++-mingw-w64-x86-64 \
    libz-mingw-w64-dev \
    libffi-dev libpng-dev p7zip-full \
    unzip zip \
    ccache curl dos2unix nano

#RUN git clone https://github.com/zbeucler2018/stable-retro stable-retro
RUN git clone https://github.com/TwitchAIRacingLeague/retro stable-retro

COPY docker/cmake cmake
COPY docker/scripts scripts

RUN dos2unix scripts/install_python.sh
RUN dos2unix scripts/build_libzip.sh
RUN dos2unix scripts/build_qt5.sh
RUN dos2unix scripts/build_ccache.sh
RUN dos2unix scripts/build_capnproto.sh



COPY third-party/libzip libzip
ARG ARCH=x86_64
ARG BITS=64
ENV PYVER=3.10

RUN CROSS=win${BITS} ROOT=/usr/${ARCH}-w64-mingw32 ./scripts/install_python.sh # Requires update to include python 3 10 6

RUN CROSS=win${BITS} ROOT=/usr/${ARCH}-w64-mingw32 ./scripts/build_libzip.sh && \
    rm -rf libzip


WORKDIR /root/stable-retro

RUN python3 -m pip install --upgrade pip wheel setuptools

#RUN python3 -m pip install -e .

RUN cmake . -DBUILD_TESTS=ON -DBUILD_MANYLINUX=ON -DPYTHON_INCLUDE_DIR=/usr/include/python3.10 -DCMAKE_TOOLCHAIN_FILE=../cmake/win64.cmake

RUN python3 setup.py -q build_ext -i -j3

RUN make -j3

RUN python3 setup.py bdist_wheel --plat-name win_amd64