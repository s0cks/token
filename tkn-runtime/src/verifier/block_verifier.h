#ifndef TKN_BLOCK_VERIFIER_H
#define TKN_BLOCK_VERIFIER_H

#include "block.h"

namespace token{
  namespace internal{
    class BlockVerifierBase{
    protected:
      BlockPtr block_;
      Hash block_hash_;
      UnclaimedTransactionPool& utxos_;

      BlockVerifierBase(const BlockPtr& blk, UnclaimedTransactionPool& utxos):
        block_(blk),
        block_hash_(blk->hash()),
        utxos_(utxos){}

      inline UnclaimedTransactionPool&
      utxos() const{
        return utxos_;
      }
    public:
      virtual ~BlockVerifierBase() = default;

      BlockPtr block() const{
        return block_;
      }

      Hash hash() const{
        return block_hash_;
      }

      virtual bool Verify() = 0;
    };
  }
}

#endif//TKN_BLOCK_VERIFIER_H