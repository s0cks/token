#include "common.h"

// BlockChain flags
DEFINE_string(path, "", "The FS path for the BlockChain");
DEFINE_uint32(heap_size, 768 * 1024 * 1024, "The size of the minor heap");
DEFINE_uint32(port, 0, "The port for the BlockChain ledger");

// Initialization Flags
DEFINE_string(snapshot, "", "The path to load a snapshot from, will reinitialize the block chain");

// Debug Flags
DEFINE_bool(enable_snapshots, false, "Enable snapshots of the block chain");