project(tkn-utils CXX)
set(CMAKE_CXX_STANDARD 11)

add_library(tkn-utils STATIC
        include/atomic/relaxed_atomic.h
        include/atomic/work_stealing_queue.h
        include/address.h src/address.cc
        include/bitfield.h
        include/bloom.h
        include/buffer.h
        include/common.h
        include/crypto.h
        include/filesystem.h src/filesystem.cc
        include/hash.h
        include/inventory.h
        include/json.h
        include/key.h
        include/keychain.h src/keychain.cc
        include/kvstore.h
        include/message.h
        include/raw_proposal.h
        include/server.h
        include/session.h
        include/string_utils.h
        include/timestamp.h
        include/trie.h
        include/units.h
        include/uuid.h
        include/version.h)
target_link_libraries(tkn-utils tkn-platform)
target_include_directories(tkn-utils PUBLIC ${PROJECT_SOURCE_DIR}/include)