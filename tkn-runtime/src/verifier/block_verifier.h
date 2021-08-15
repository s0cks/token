#ifndef TKN_BLOCK_VERIFIER_H
#define TKN_BLOCK_VERIFIER_H

#include "block.h"

namespace token{
  namespace internal{
    class BlockVerifierBase{
    protected:
      BlockPtr blk_;
      Hash blk_hash_;

      BlockVerifierBase(const Hash& hash, BlockPtr val):
        blk_(std::move(val)),
        blk_hash_(hash){}
    public:
      virtual ~BlockVerifierBase() = default;

      BlockPtr block() const{
        return blk_;
      }

      Hash hash() const{
        return blk_hash_;
      }

      virtual bool Verify() = 0;
    };
  }
}

#endif//TKN_BLOCK_VERIFIER_H