FROM ubuntu:18.04
ARG GITHUB_TOKEN
ARG TOKEN_VERSION

ENV CMAKE_VERSION=3.17.2
ENV NODE_VERSION=12.15.0
ENV GTEST_VERSION=release-1.10.0
ENV HTTP_PARSER_VERSION=v2.9.4
ENV ZXING_VERSION=v1.1.1
ENV CRYPTOPP_VERSION=CRYPTOPP_8_4_0
ENV RAPIDJSON_VERSION=v1.1.0
ENV LEVELDB_VERSION=1.22

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
    libdevmapper-dev \
    libpopt-dev \
    libgcrypt20-dev \
    libuuid1 \
    libcrypto++-dev \
    libconfig++-dev \
    libgflags-dev \
    libgoogle-glog-dev \
    libuv1-dev \
    uuid-dev

RUN useradd -ms /bin/bash token
RUN git config --global url."https://${GITHUB_TOKEN}@github.com/".insteadOf "https://github.com/"

# Build CMake
RUN wget https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-Linux-x86_64.sh -q -O /tmp/cmake-install.sh \
  && chmod u+x /tmp/cmake-install.sh \
  && mkdir /opt/cmake \
  && /tmp/cmake-install.sh --skip-license --prefix=/opt/cmake \
  && rm /tmp/cmake-install.sh
ENV PATH="/opt/cmake/bin:${PATH}"

# Build and Install GTest
RUN git clone https://github.com/google/googletest.git -b ${GTEST_VERSION} \
 && cd googletest \
 && mkdir -p build \
 && cd build \
 && cmake .. \
 && cmake --build . --target install

# Build Crypto++
RUN git clone https://github.com/weidai11/cryptopp \
  && cd cryptopp \
  && make libcryptopp.so \
  && make install PREFIX=/usr/local

# Build and Install HttpParser
RUN git clone https://github.com/nodejs/http-parser -b ${HTTP_PARSER_VERSION} \
 && cd http-parser \
 && make \
 && make install PREFIX=/usr/local

# Build & Install ZXing-cpp
RUN git clone https://github.com/nu-book/zxing-cpp.git -b ${ZXING_VERSION} \
 && cd zxing-cpp \
 && mkdir build/ \
 && cd build/ \
 && cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/usr/local/ .. \
 && cmake --build . --target install

# Build & Install rapidjson
RUN git clone https://github.com/Tencent/rapidjson.git -b ${RAPIDJSON_VERSION} \
 && cp -r rapidjson/include/rapidjson /usr/local/include/ \
 && ls -lash /usr/local/include/

# Build & Install LevelDB
RUN git clone --recurse-submodules https://github.com/google/leveldb.git -b ${LEVELDB_VERSION} \
 && cd leveldb/ \
 && mkdir build/ \
 && cd build/ \
 && cmake -DCMAKE_BUILD_TYPE=Release .. \
 && cmake --build . --target install

# Copy The Ledger Source
RUN mkdir -p /usr/src/libtoken-ledger/build
COPY cmake /usr/src/libtoken-ledger/cmake/
COPY include /usr/src/libtoken-ledger/include/
COPY src /usr/src/libtoken-ledger/src/
COPY tests /usr/src/libtoken-ledger/tests/
COPY CMakeLists.txt main.cc client.cc tests.cc inspector.cc /usr/src/libtoken-ledger/
WORKDIR /usr/src/libtoken-ledger/build
RUN cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr/local/ ..\
 && cmake --build . --target install \
 && ldconfig

USER root
CMD [ "token-node", "--path", "/usr/share/ledger", "--service-port", "8080", "--server-port", "8081", "--healthcheck-port", "8082", "--miner-interval", "30000" ]

# Expose the REST Service
EXPOSE 8080
# Expose the RPC Service
EXPOSE 8081
# Expose the HealthCheck Service
EXPOSE 8082