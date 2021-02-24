#include <gtest/gtest.h>
#include "tests/test_suite.h"

// --path "/usr/share/ledger"
DEFINE_string(path, "", "The path for the local ledger to be stored in.");
// --enable-snapshots
DEFINE_bool(enable_snapshots, false, "Enable snapshots of the block chain");
// --num-workers 10
DEFINE_int32(num_workers, 4, "Define the number of worker pool threads");
// --miner-interval 3600
DEFINE_int64(miner_interval, 1000 * 60 * 1, "The amount of time between mining blocks in milliseconds.");

#ifdef TOKEN_DEBUG
  // --fresh
  DEFINE_bool(fresh, false, "Initialize the BlockChain w/ a fresh chain [Debug]");
  // --append-test
  DEFINE_bool(append_test, false, "Append a test block upon startup [Debug]");
  // --verbose
  DEFINE_bool(verbose, false, "Turn on verbose logging [Debug]");
  // --no-mining
  DEFINE_bool(no_mining, false, "Turn off block mining [Debug]");
#endif//TOKEN_DEBUG

#ifdef TOKEN_ENABLE_SERVER
  // --remote localhost:8080
  DEFINE_string(remote, "", "The hostname for the remote ledger to synchronize with.");
  // --rpc-port 8080
  DEFINE_int32(server_port, 0, "The port for the ledger RPC service.");
  // --num-peers 12
  DEFINE_int32(num_peers, 4, "The max number of peers to connect to.");
#endif//TOKEN_ENABLE_SERVER

#ifdef TOKEN_ENABLE_HEALTH_SERVICE
  // --healthcheck-port 8081
  DEFINE_int32(healthcheck_port, 0, "The port for the ledger health check service.");
#endif//TOKEN_ENABLE_HEALTHCHECK

#ifdef TOKEN_ENABLE_REST_SERVICE
  // --service-port 8082
  DEFINE_int32(service_port, 0, "The port for the ledger controller service.");
#endif//TOKEN_ENABLE_REST_SERVICE

int
main(int argc, char **argv){
  ::google::SetStderrLogging(google::INFO);
  ::google::InitGoogleLogging(argv[0]);
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::InitGoogleMock(&argc, argv);
  return RUN_ALL_TESTS();
}