#include "unclaimed_transaction.h"
#include "byte_buffer.h"
#include "block_chain.h"
#include "crash_report.h"

namespace Token{
//######################################################################################################################
//                                          Unclaimed Transaction
//######################################################################################################################
    Handle<UnclaimedTransaction> UnclaimedTransaction::NewInstance(ByteBuffer* bytes){
        Hash hash = bytes->GetHash();
        uint32_t index = bytes->GetInt();
        User user = User(bytes);
        Product product = Product(bytes);
        return new UnclaimedTransaction(hash, index, user, product);
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
}
