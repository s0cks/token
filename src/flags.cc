#include "common.h"

// BlockChain flags
DEFINE_string(path, "", "The FS path for the BlockChain");
DEFINE_uint32(heap_size, 768 * 1024 * 1024, "The size of the minor heap");
DEFINE_uint32(port, 0, "The port for the BlockChain ledger");

DEFINE_string(peer_address, "", "The address for the peer to connect to");
DEFINE_uint32(peer_port, 0, "The port for the peer to connect to");