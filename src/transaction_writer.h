#ifndef TOKEN_TRANSACTION_WRITER_H
#define TOKEN_TRANSACTION_WRITER_H

#include "transaction.h"
#include "file_writer.h"

namespace Token{
    class TransactionFileWriter : public BinaryFileWriter{
    private:
        inline bool
        WriteInput(const Input& value){
            WriteHash(value.GetTransactionHash());
            WriteInt(value.GetOutputIndex());
            WriteUser(value.GetUser());
            return true;
        }

        inline bool
        WriteOutput(const Output& value){
            WriteUser(value.GetUser());
            WriteProduct(value.GetProduct());
            return true;
        }
    public:
        TransactionFileWriter(const std::string& filename):
            BinaryFileWriter(filename){}
        TransactionFileWriter(BinaryFileWriter* parent):
            BinaryFileWriter(parent){}
        ~TransactionFileWriter() = default;

        bool Write(const TransactionPtr& tx){
            //TODO: better error handling
            WriteLong(tx->GetTimestamp()); // timestamp_
            WriteLong(tx->GetIndex()); // index_
            WriteLong(tx->GetNumberOfInputs()); // num_inputs_
            for(auto& it : tx->inputs())
                WriteInput(it); // input_[idx]
            WriteLong(tx->GetNumberOfOutputs()); // num_outputs_
            for(auto& it : tx->outputs())
                WriteOutput(it); // outputs_[idx]
            //TODO: serialize transaction signature
            return true;
        }
    };
}

#endif //TOKEN_TRANSACTION_WRITER_H
