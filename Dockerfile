FROM token-base:0.0.5
# Install Dependencies
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

# Copy The Ledger Source
RUN mkdir -p /usr/src/libtoken-ledger/build
COPY cmake /usr/src/libtoken-ledger/cmake/
COPY include /usr/src/libtoken-ledger/include/
COPY src /usr/src/libtoken-ledger/src/
COPY tests /usr/src/libtoken-ledger/tests/
COPY CMakeLists.txt main.cc client.cc tests.cc inspector.cc /usr/src/libtoken-ledger/

# Create the Token User
RUN useradd token \
    && passwd -d token

# Create the Ledger Directory
RUN mkdir -p /opt/token/ledger \
    && chown token:token /opt/token/ledger

# Compile the Ledger Source
WORKDIR /usr/src/libtoken-ledger/build
RUN cmake -DCMAKE_BUILD_TYPE=Debug .. \
    && cmake --build . --target install

CMD [ "./token-node", "--path", "/opt/token/ledger", "--port", "8000" ]
# TODO: Update and get working
# TODO: Investigate using vagrant to test docker images?