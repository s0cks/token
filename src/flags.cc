#include "common.h"

// BlockChain flags
DEFINE_string(path, "", "The FS path for the BlockChain");
DEFINE_bool(verbose, false, "Set whether or not the BlockChain should be more verbose in the logs");
DEFINE_uint32(minheap_size, 1024 * 20, "The size of the minor heap");
DEFINE_uint32(maxheap_size, 1024 * 20 * 5, "The size of the major heap");
DEFINE_uint32(port, 0, "The port for the BlockChain ledger");
DEFINE_string(peers, "", "The comma-delimited list of peers to use");

DEFINE_uint32(service_port, 0, "The port for the BlockChain RPC Service");