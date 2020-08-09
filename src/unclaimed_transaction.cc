#include "unclaimed_transaction.h"
#include "bytes.h"
#include "block_chain.h"
#include "crash_report.h"

namespace Token{
//######################################################################################################################
//                                          Unclaimed Transaction
//######################################################################################################################
    Handle<UnclaimedTransaction> UnclaimedTransaction::NewInstance(uint8_t* bytes){
        size_t offset = 0;
        uint256_t hash = DecodeHash(&bytes[offset]);
        offset += uint256_t::kSize;

        uint32_t index = DecodeInt(&bytes[offset]);
        offset += 4;

        uint32_t user_length = DecodeInt(&bytes[offset]);
        offset += 4;

        std::string user = DecodeString(&bytes[offset], user_length);
        return new UnclaimedTransaction(hash, index, user);
    }

    Handle<UnclaimedTransaction> UnclaimedTransaction::NewInstance(std::fstream& fd){
        //TODO: implement
        return nullptr;
    }

    std::string UnclaimedTransaction::ToString() const{
        std::stringstream stream;
        stream << "UnclaimedTransaction(" << hash_ << "[" << index_ << "])";
        return stream.str();
    }

    size_t UnclaimedTransaction::GetBufferSize() const{
        size_t size = 0;
        size += uint256_t::kSize;
        size += 4;
        size += (4 + user_.length());
        return size;
    }

    bool UnclaimedTransaction::Encode(uint8_t* bytes) const{
        size_t offset = 0;
        EncodeHash(&bytes[offset], hash_);
        offset += uint256_t::kSize;

        EncodeInt(&bytes[offset], index_);
        offset += 4;

        EncodeString(&bytes[offset], user_);
        return true;
    }
}
