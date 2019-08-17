# Dependencies

- crypto++
- Google's Protocol Buffers
- gRPC
- glog
- libuv
- leveldb
- sqlite3
- rapidjson <--- Removing


# Building

## Prerequisites

Building will require the dependencies discussed above and CMake

## Compiling

```bash
mkdir build
cd build/
cmake ..
cmake --build .
```