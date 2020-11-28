#include "unclaimed_transaction.h"
#include "buffer.h"
#include "crash_report.h"

namespace Token{
    Handle<UnclaimedTransaction> UnclaimedTransaction::NewInstance(const Handle<Buffer>& buff){
        Hash hash = buff->GetHash();
        int32_t index = buff->GetInt();
        User user = buff->GetUser();
        Product product = buff->GetProduct();
        return new UnclaimedTransaction(hash, index, user, product);
    }

    Handle<UnclaimedTransaction> UnclaimedTransaction::NewInstance(std::fstream& fd, size_t size){
        Handle<Buffer> buff = Buffer::NewInstance(size);
        fd.read((char*)buff->data(), size);
        return NewInstance(buff);
    }

    std::string UnclaimedTransaction::ToString() const{
        std::stringstream stream;
        stream << "UnclaimedTransaction(" << hash_ << "[" << index_ << "])";
        return stream.str();
    }
}
