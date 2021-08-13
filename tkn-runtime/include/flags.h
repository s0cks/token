#ifndef TOKEN_FLAGS_H
#define TOKEN_FLAGS_H

#include <gflags/gflags.h>
#include <glog/logging.h>

// core
DECLARE_string(path);
DECLARE_int32(num_workers);
DECLARE_uint64(mining_interval);
DECLARE_uint64(proposal_timeout);
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
DECLARE_string(elasticsearch_hostname);
#endif//TOKEN_ENABLE_EXPERIMENTAL

#ifdef TOKEN_DEBUG
DECLARE_uint64(spend_test_min);
DECLARE_uint64(spend_test_max);
#endif//TOKEN_DEBUG

namespace token{
  static inline bool
  IsMiningEnabled(){
    return FLAGS_mining_interval > 0;
  }

  static inline bool
  IsServerEnabled(){
    return FLAGS_server_port > 0;
  }

  static inline bool
  IsRestServiceEnabled(){
    return FLAGS_service_port > 0;
  }

  static inline bool
  IsHealthServiceEnabled(){
    return FLAGS_healthcheck_port > 0;
  }

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
  ValidateFlagMiningInterval(const char* ss, uint64_t val){
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

  static inline bool
  ValidateProposalTimeout(const char** ss, uint64_t val){
    return true;
  }
}

#define TOKEN_BLOCKCHAIN_HOME (FLAGS_path)
#define TOKEN_VERBOSE (FLAGS_verbose)

#endif//TOKEN_FLAGS_H