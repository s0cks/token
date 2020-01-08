#ifndef TOKEN_FLAGS_H
#define TOKEN_FLAGS_H

#include <gflags/gflags.h>

// BlockChain Flags
DECLARE_string(path);
DECLARE_bool(verbose);
DECLARE_uint32(minheap_size);
DECLARE_uint32(maxheap_size);

// RPC Service Flags
DECLARE_uint32(service_port);

#define TOKEN_VERBOSE (FLAGS_verbose)
#define TOKEN_BLOCKCHAIN_HOME (FLAGS_path)

#endif //TOKEN_FLAGS_H