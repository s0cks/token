#include "common.h"

DEFINE_string(path, "", "The path for the local ledger to be stored in.");
DEFINE_bool(enable_snapshots, false, "Enable snapshots of the block chain");

#ifdef TOKEN_ENABLE_SERVER
  DEFINE_string(remote, "", "The hostname for the remote ledger to synchronize with.");
  DEFINE_int32(server_port, 0, "The port for the ledger RPC service.");
#endif//TOKEN_ENABLE_SERVER

#ifdef TOKEN_ENABLE_HEALTH_SERVICE
  DEFINE_int32(healthcheck_port, 0, "The port for the ledger health check service.");
#endif//TOKEN_ENABLE_HEALTHCHECK

#ifdef TOKEN_ENABLE_REST_SERVICE
  DEFINE_int32(service_port, 0, "The port for the ledger rest service.");
#endif//TOKEN_ENABLE_REST