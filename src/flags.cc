#include "common.h"

DEFINE_string(path, "", "The path for the local ledger to be stored in.");
DEFINE_bool(enable_snapshots, false, "Enable snapshots of the block chain");
DEFINE_int32(num_workers, 4, "Define the number of worker pool threads");
DEFINE_int64(miner_interval, 1000 * 60 * 1, "The amount of time between mining blocks in milliseconds.");

#ifdef TOKEN_DEBUG
DEFINE_bool(append_test, false, "Append a test block upon startup [Debug]");
#endif//TOKEN_DEBUG

#ifdef TOKEN_ENABLE_SERVER
DEFINE_string(remote, "", "The hostname for the remote ledger to synchronize with.");
DEFINE_int32(server_port, 0, "The port for the ledger RPC service.");
DEFINE_int32(num_peers, 4, "The max number of peers to connect to.");
#endif//TOKEN_ENABLE_SERVER

#ifdef TOKEN_ENABLE_HEALTH_SERVICE
DEFINE_int32(healthcheck_port, 0, "The port for the ledger health check service.");
#endif//TOKEN_ENABLE_HEALTHCHECK

#ifdef TOKEN_ENABLE_REST_SERVICE
DEFINE_int32(service_port, 0, "The port for the ledger rest service.");
#endif//TOKEN_ENABLE_REST