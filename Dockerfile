FROM ubuntu:18.04
ARG GITHUB_TOKEN
ARG TOKEN_VERSION

ENV CMAKE_VERSION=3.17.2
ENV NODE_VERSION=12.15.0
ENV GTEST_VERSION=release-1.10.0
ENV HTTP_PARSER_VERSION=v2.9.4
ENV JSONLIB_VERSION=1.9.4

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
    libleveldb-dev \
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
#RUN git clone https://github.com/weidai11/cryptopp \
#  && cd cryptopp \
#  && make libcryptopp.so \
#  && make install PREFIX=/usr/local

# Build and Install HttpParser
RUN git clone https://github.com/nodejs/http-parser -b ${HTTP_PARSER_VERSION} \
 && cd http-parser \
 && make \
 && make install PREFIX=/usr/local

# Build and Install jsoncpp
RUN git clone https://github.com/open-source-parsers/jsoncpp.git -b ${JSONLIB_VERSION} \
 && cd jsoncpp \
 && mkdir -p build \
 && cd build \
 && cmake -DBUILD_STATIC_LIBS=OFF -DBUILD_SHARED_LIBS=ON .. \
 && cmake --build . --target install

# Build and Install the Token Ledger
#RUN git clone https://github.com/tokenevents/libtoken-ledger.git -b $TOKEN_VERSION \
# && cd libtoken-ledger \
# && mkdir -p build \
# && cd build \
# && cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr/local .. \
# && cmake --build . --target install \
# && ldconfig

# Copy The Ledger Source
RUN mkdir -p /usr/src/libtoken-ledger/build
COPY cmake /usr/src/libtoken-ledger/cmake/
COPY include /usr/src/libtoken-ledger/include/
COPY src /usr/src/libtoken-ledger/src/
COPY tests /usr/src/libtoken-ledger/tests/
COPY CMakeLists.txt main.cc client.cc tests.cc inspector.cc /usr/src/libtoken-ledger/
WORKDIR /usr/src/libtoken-ledger/build
RUN cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_INSTALL_PREFIX=/usr/local ..\
 && cmake --build . --target install \
 && ldconfig

USER root
CMD [ "token-node", "--path", "/usr/share/ledger", "--port", "8080" ]
# Expose the RPC Service
EXPOSE 8080
# Expose the HealthCheck Service
EXPOSE 8081