project(tkn-utils CXX)
set(CMAKE_CXX_STANDARD 11)

find_package(glog REQUIRED)

add_library(tkn-utils STATIC
  include/atomic/bit_vector.h
  include/atomic/linked_list.h
  include/atomic/relaxed_atomic.h
  include/atomic/work_stealing_queue.h

  include/server/message.h
  include/server/server.h
  include/server/session.h

  include/address.h src/address.cc
  include/bitfield.h
  include/bloom.h
  include/buffer.h
  include/common.h
  include/crypto.h
  include/dbutils.h
  include/hash.h
  include/inventory.h
  include/json.h
  include/key.h
  include/keychain.h src/keychain.cc
  include/kvstore.h
  include/loop_thread.h
  include/raw_proposal.h
  include/string_utils.h
  include/timer.h
  include/timestamp.h
  include/trie.h
  include/units.h
  include/uuid.h
  include/version.h
  include/builder.h
  include/eventbus.h src/eventbus.cc include/array.h)
target_link_libraries(tkn-utils
  tkn-platform
  ${GLOG_LIBRARIES})
target_include_directories(tkn-utils PUBLIC ${PROJECT_SOURCE_DIR}/include)