#include <mutex>
#include <condition_variable>
#include <leveldb/db.h>
#include "utils/buffer.h"
#include "unclaimed_transaction.h"

namespace Token{
    UnclaimedTransactionPtr UnclaimedTransaction::NewInstance(std::fstream& fd, size_t size){
        Buffer buff(size);
        fd.read((char*)buff.data(), size);
        return NewInstance(&buff);
    }

    std::string UnclaimedTransaction::ToString() const{
        std::stringstream stream;
        stream << "UnclaimedTransaction(" << hash_ << "[" << index_ << "])";
        return stream.str();
    }
}