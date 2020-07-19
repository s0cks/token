#include "common.h"

// BlockChain flags
DEFINE_string(path, "", "The FS path for the BlockChain");
DEFINE_bool(verbose, false, "Set whether or not the BlockChain should be more verbose in the logs");
DEFINE_uint64(heap_size, 512 * 1024 * 1024, "The size of the minor heap");
DEFINE_uint32(port, 0, "The port for the BlockChain ledger");
DEFINE_string(peer_address, "", "The address for the peer to connect to");
DEFINE_uint32(peer_port, 0, "The port for the peer to connect to");

DEFINE_uint32(service_port, 0, "The port for the BlockChain RPC Service");