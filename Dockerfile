FROM token-base:0.0.1
# Install Latest Dependencies
RUN apt-get update && apt-get install -y \
    libdevmapper-dev \
    libprotoc-dev \
    libpopt-dev \
    libgcrypt20-dev \
    protobuf-compiler \
    libuuid1 \
    libcrypto++-dev \
    libconfig++-dev \
    libgflags-dev \
    libgoogle-glog-dev \
    libprotobuf-dev \
    libleveldb-dev \
    libuv1-dev \
    uuid-dev

# Debug
RUN echo "$(ls -lash /usr/include/)"
RUN echo "$(ls -lash /usr/local/include/)"

# Copy + Compile Ledger Source
RUN mkdir /token-node/
COPY cmake /token-node/cmake
COPY include /token-node/include
COPY protos /token-node/protos
COPY src /token-node/src
COPY CMakeLists.txt main.cc client.cc tests.cc /token-node/
WORKDIR /token-node
RUN cmake -DCMAKE_BUILD_TYPE=Debug . && make

# Run the Ledger Executable
WORKDIR /opt/token-node
COPY /token-node/token-node ./
CMD [ "./token-node" ]
# TODO: Update and get working
# TODO: Investigate using vagrant to test docker images?