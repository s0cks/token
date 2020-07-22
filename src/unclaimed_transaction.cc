#include "unclaimed_transaction.h"
#include "block_chain.h"
#include "crash_report.h"

namespace Token{
//######################################################################################################################
//                                          Unclaimed Transaction
//######################################################################################################################
    Handle<UnclaimedTransaction> UnclaimedTransaction::NewInstance(const uint256_t &hash, uint32_t index, const std::string& user){
        return new UnclaimedTransaction(hash, index, user);
    }

    Handle<UnclaimedTransaction> UnclaimedTransaction::NewInstance(const UnclaimedTransaction::RawType& raw){
        return NewInstance(HashFromHexString(raw.tx_hash()), raw.tx_index(), raw.user());
    }

    Handle<UnclaimedTransaction> UnclaimedTransaction::NewInstance(std::fstream& fd){
        UnclaimedTransaction::RawType raw;
        if(!raw.ParseFromIstream(&fd)) return nullptr;
        return NewInstance(raw);
    }

    std::string UnclaimedTransaction::ToString() const{
        std::stringstream stream;
        stream << "UnclaimedTransaction(" << hash_ << "[" << index_ << "])";
        return stream.str();
    }

    bool UnclaimedTransaction::WriteToMessage(RawType& raw) const{
        raw.set_tx_hash(HexString(hash_));
        raw.set_tx_index(index_);
        raw.set_user(user_);
        return true;
    }
}
