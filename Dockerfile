FROM grpc/cxx
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    libcrypto++-dev \
    libprotobuf-dev \
    libleveldb-dev \
    libgoogle-glog-dev \
    libgflags-dev \
    libuv1-dev \
    libhttp-parser-dev
COPY build/token-0.1.1-Linux.deb /tmp/token-0.1.1-Linux.deb
RUN dpkg -i /tmp/token-0.1.1-Linux.deb
# TODO: Update and get working
# TODO: Investigate using vagrant to test docker images?