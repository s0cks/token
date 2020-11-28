# TODO:
# - change the base image to something more compact
# - install the following dependencies from source:
#       * uuid-dev
#       * https://github.com/libuv/libuv
#       * https://github.com/hyperrealm/libconfig
#       * https://github.com/weidai11/cryptopp
#       * https://github.com/gflags/gflags
#       * https://github.com/google/glog
#       * https://github.com/google/leveldb
FROM ubuntu:18.04
ENV NODE_VERSION=12.15.0

RUN apt-get update && apt-get install -y \
    autoconf \
    automake \
    build-essential \
    pkg-config \
    curl \
    git \
    wget \
    unzip \
    libtool \
    libgtest-dev \
    libdevmapper-dev \
    libpopt-dev \
    libgcrypt20-dev \
    libuuid1 \
    libcrypto++-dev \
    libconfig++-dev \
    libgflags-dev \
    libgoogle-glog-dev \
    libleveldb-dev \
    libuv1-dev \
    uuid-dev

# Build CMake
RUN wget https://github.com/Kitware/CMake/releases/download/v3.17.2/cmake-3.17.2-Linux-x86_64.sh \
      -q -O /tmp/cmake-install.sh \
      && chmod u+x /tmp/cmake-install.sh \
      && mkdir /opt/cmake \
      && /tmp/cmake-install.sh --skip-license --prefix=/opt/cmake \
      && rm /tmp/cmake-install.sh
ENV PATH="/opt/cmake/bin:${PATH}"

# Build Crypto++
RUN git clone https://github.com/weidai11/cryptopp \
 && cd cryptopp \
 && make libcryptopp.a libcryptopp.so cryptest.exe \
 && make install PREFIX=/usr/local

# Build Http Parser
RUN git clone https://github.com/nodejs/http-parser \
 && cd http-parser \
 && make \
 && make install PREFIX=/usr/local

# Build GTest Library
WORKDIR /usr/src/googletest
RUN cmake . \
    && cmake --build . --target install

# Build and Install NodeJS
RUN curl -sL https://deb.nodesource.com/setup_lts.x | bash - \
 && apt-get install -y nodejs

# Copy The Ledger Source
RUN mkdir -p /usr/src/libtoken-ledger/build
COPY cmake /usr/src/libtoken-ledger/cmake/
COPY include /usr/src/libtoken-ledger/include/
COPY src /usr/src/libtoken-ledger/src/
COPY tests /usr/src/libtoken-ledger/tests/
COPY CMakeLists.txt main.cc client.cc tests.cc inspector.cc /usr/src/libtoken-ledger/

# Compile the Ledger Source
WORKDIR /usr/src/libtoken-ledger/build
RUN cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr/local ..\
    && cmake --build . --target install \
    && ldconfig