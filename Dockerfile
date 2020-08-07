FROM token-base:0.0.1
# Install Latest Dependencies
RUN apt-get update && apt-get install -y \
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

# Copy + Compile Ledger Source
RUN mkdir /token-node/
COPY cmake /token-node/cmake
COPY include /token-node/include
COPY protos /token-node/protos
COPY src /token-node/src
COPY tests /token-node/tests
COPY CMakeLists.txt main.cc client.cc tests.cc /token-node/
WORKDIR /token-node

RUN mkdir -p /opt/token \
    && cmake -DCMAKE_BUILD_TYPE=Debug . -DCMAKE_INSTALL_PREFIX=/opt/token/ \
    && cmake --build . --target install

WORKDIR /opt/token/
CMD [ "./token-node" ]
# TODO: Update and get working
# TODO: Investigate using vagrant to test docker images?