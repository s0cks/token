#ifndef TOKEN_BLOCK_WRITER_H
#define TOKEN_BLOCK_WRITER_H

#include "block.h"
#include "file_writer.h"
#include "transaction_writer.h"

namespace Token{
    class BlockFileWriter : public BinaryFileWriter{
    private:
        TransactionFileWriter tx_writer_;
    public:
        BlockFileWriter(const std::string& filename):
            BinaryFileWriter(filename),
            tx_writer_(this){}
        ~BlockFileWriter() = default;

        bool Write(const BlockPtr& blk){
            WriteLong(blk->GetTimestamp()); // timestamp_
            WriteLong(blk->GetHeight()); // height_
            WriteHash(blk->GetPreviousHash()); // previous_hash_
            WriteLong(blk->GetNumberOfTransactions()); // num_transactions_
            for(auto& it : blk->transactions())
                tx_writer_.Write(it); // transactions_[idx]
            return true;
        }
    };
}

#endif //TOKEN_BLOCK_WRITER_H