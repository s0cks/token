#include "unclaimed_transaction.h"
#include "bytes.h"
#include "block_chain.h"
#include "crash_report.h"

namespace Token{
//######################################################################################################################
//                                          Unclaimed Transaction
//######################################################################################################################
    Handle<UnclaimedTransaction> UnclaimedTransaction::NewInstance(ByteBuffer* bytes){
        uint256_t hash = bytes->GetHash();
        uint32_t index = bytes->GetInt();
        std::string user = bytes->GetString();
        return new UnclaimedTransaction(hash, index, user);
    }

    Handle<UnclaimedTransaction> UnclaimedTransaction::NewInstance(std::fstream& fd, size_t size){
        ByteBuffer bytes(size);
        fd.read((char*)bytes.data(), size);
        return NewInstance(&bytes);
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

    bool UnclaimedTransaction::Encode(ByteBuffer* bytes) const{
        bytes->PutHash(hash_);
        bytes->PutInt(index_);
        bytes->PutString(user_);
        return true;
    }
}
