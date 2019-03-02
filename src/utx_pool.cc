#include "utx_pool.h"

namespace Token{
    BlockChain* UnclaimedTransaction::GetChain() const{
        return GetOwner()->GetChain();
    }
}