# Token Ledger

This project contains the native code (C++) that runs the Token Distributed Ledger

# Building

## Dependencies

In order to compile this project, you will need the following libraries available:
- crypto++
- Google's Protocol Buffers
- gRPC
- glog
- leveldb
- sqlite3

## Compiling

```bash
# Create build directory
mkdir build && cd build

# Release builds
cmake -DCMAKE_BUILD_TYPE=Release ..
# Debug builds
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Compile
cmake --build .
```

# Other

# Starting the Local Ledger

```bash
./token-node --help

  -maxheap_size (The size of the major heap) type: uint32 default: 20480
  -minheap_size (The size of the minor heap) type: uint32 default: 2560
  -path (The FS path for the BlockChain) type: string default: ""
  -peer_port (The port to connect to a peer with) type: uint32 default: 0
  -server_port (The port used for the BlockChain server) type: uint32
    default: 0
  -service_port (The port used for the RPC service) type: uint32 default: 0
  -help (show help on all flags [tip: all flags can have two dashes])
    type: bool default: false currently: true
  -helpfull (show help on all flags -- same as -help) type: bool
    default: false
  -helpmatch (show help on modules whose name contains the specified substr)
    type: string default: ""
  -helpon (show help on the modules named by this flag value) type: string
    default: ""
  -helppackage (show help on all modules in the main package) type: bool
    default: false
  -helpshort (show help on only the main module for this program) type: bool
    default: false
  -helpxml (produce an xml version of help) type: bool default: false
  -version (show version and build info and exit) type: bool default: false
```
 
## Cleaning a Local Ledger

```bash
sh clean.sh /your/path/to/ledger
```