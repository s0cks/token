#ifndef TOKEN_FLAGS_H
#define TOKEN_FLAGS_H

#include <gflags/gflags.h>
#include <glog/logging.h>

// core
DECLARE_string(path);
DECLARE_int32(num_workers);
DECLARE_int64(mining_interval);
DECLARE_bool(reinitialize);

// server
DECLARE_string(remote);
DECLARE_int32(server_port);
DECLARE_int32(num_peers);

// services
DECLARE_int32(healthcheck_port);
DECLARE_int32(service_port);

// experimental
#ifdef TOKEN_ENABLE_EXPERIMENTAL
DECLARE_bool(enable_snapshots);
#endif//TOKEN_ENABLE_EXPERIMENTAL

#ifdef TOKEN_DEBUG
DECLARE_bool(append_test);
#endif//TOKEN_DEBUG

namespace token{
  static inline bool
  ValidateFlagPath(const char* ss, const std::string& val){
    return true;
  }

  static inline bool
  ValidateFlagNumberOfWorkers(const char* ss, int32_t val){
    DLOG(INFO) << "ss: " << ss;
    DLOG(INFO) << "val: " << val;
    return true;
  }

  static inline bool
  ValidateFlagMiningInterval(const char* ss, int64_t val){
    return true;
  }

  static inline bool
  ValidateFlagRemote(const char* ss, const std::string& remote){
    return true;
  }

  static inline bool
  ValidateFlagPort(const char* ss, int32_t val){
    return true;
  }

  static inline bool
  ValidateFlagNumberOfPeers(const char* ss, int32_t val){
    return true;
  }
}

#define TOKEN_BLOCKCHAIN_HOME (FLAGS_path)
#define TOKEN_VERBOSE (FLAGS_verbose)

#endif//TOKEN_FLAGS_H