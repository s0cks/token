#include <glog/logging.h>
#include "block_chain.h"
#include "block.h"

namespace Token{
    std::string Block::GetHash(){
        CryptoPP::SHA256 func;
        std::string digest;
        Messages::Block raw;
        raw << this;
        size_t size = raw.ByteSizeLong();
        uint8_t bytes[size];
        if(!raw.SerializeToArray(bytes, size)){
            LOG(WARNING) << "couldn't serialize block to byte array";
            return ""; //TODO: return invalid hash;
        }
        CryptoPP::ArraySource source(bytes, size, true, new CryptoPP::HashFilter(func, new CryptoPP::HexEncoder(new CryptoPP::StringSink(digest))));
        return digest;
    }

    bool Block::Accept(BlockVisitor* vis){
        if(!vis->VisitBlockStart()) return false;
        for(size_t idx = 0;
            idx < GetNumberOfTransactions();
            idx++){
            Transaction* tx;
            if(!(tx = GetTransactionAt(idx))) return false;
            if(!vis->VisitTransaction(tx)){
                return false;
            }
        }
        return vis->VisitBlockEnd();
    }
}