#include "common.h"
#include "alloc/allocator.h"

// BlockChain flags
DEFINE_string(path, "", "The FS path for the BlockChain");

#ifndef TOKEN_GCMODE_NONE
DEFINE_string(heap_size, Token::Allocator::kDefaultHeapSizeAsString, "The size of the heap. ie 3m, 5g, 100g, 421m, 12374743b, etc...");
#endif//!TOKEN_GCMODE_NONE

DEFINE_uint32(port, 0, "The port for the BlockChain ledger");
DEFINE_string(peer, "", "The address of the peer to connect to on boot");

// Initialization Flags
DEFINE_string(snapshot, "", "The path to load a snapshot from, will reinitialize the block chain");

// Debug Flags
DEFINE_bool(enable_snapshots, false, "Enable snapshots of the block chain");