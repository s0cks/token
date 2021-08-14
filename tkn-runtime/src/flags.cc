#include "flags.h"

// core
DEFINE_string(path, "", "The path for the local ledger to be stored in.");
DEFINE_int32(num_workers, 4, "Define the number of worker pool threads");
DEFINE_uint64(mining_interval, 1000 * 60 * 1, "The amount of time between mining blocks in milliseconds.");
DEFINE_uint64(proposal_timeout, 1000 * 60 * 10, "The proposal timeout in milliseconds");
DEFINE_bool(reinitialize, false, "Reinitialize the block chain");
DEFINE_bool(enable_async, false, "Enable async block verification & commit");

// server
DEFINE_string(remote, "", "The hostname for the remote ledger to join.");
DEFINE_int32(server_port, 0, "The port to use for the RPC serer.");
DEFINE_int32(num_peers, 2, "The max number of peers to connect to.");

// services
DEFINE_int32(healthcheck_port, 0, "The port to use for the health check service.");
DEFINE_int32(service_port, 0, "The port for the ledger controller service.");

#ifdef TOKEN_ENABLE_EXPERIMENTAL
DEFINE_bool(enable_snapshots, false, "Enable snapshots of the block chain");
#endif//TOKEN_ENABLE_EXPERIMENTAL

#ifdef TOKEN_DEBUG
DEFINE_uint64(spend_test_min, 0, "[Test] The minimum amount of tokens to be spent");
DEFINE_uint64(spend_test_max, 0, "[Test] The maximum amount of tokens to be spent");
DEFINE_string(elasticsearch_hostname, "localhost:8080", "The elasticsearch hostname.");
#endif//TOKEN_DEBUG