#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include <set>
#include "hash.h"
#include "object.h"
#include "unclaimed_transaction.h"

#include "utils/printer.h"
#include "utils/file_reader.h"
#include "utils/file_writer.h"

namespace Token{
    typedef std::shared_ptr<Transaction> TransactionPtr;

    class Input : public Object{
        friend class Transaction;
    private:
        Hash hash_;
        int32_t index_;
        User user_;
    protected:
        bool Encode(Buffer* buff) const{
            buff->PutHash(hash_);
            buff->PutInt(index_);
            buff->PutUser(user_);
            return true;
        }
    public:
        Input(const Hash& tx_hash, int32_t index, const User& user):
            Object(),
            hash_(tx_hash),
            index_(index),
            user_(user){}
        Input(const Hash& tx_hash, int32_t index, const std::string& user):
            Input(tx_hash, index, User(user)){}
        Input(Buffer* buff):
            Object(),
            hash_(buff->GetHash()),
            index_(buff->GetInt()),
            user_(buff->GetUser()){}
        ~Input(){}

        int32_t GetOutputIndex() const{
            return index_;
        }

        Hash GetTransactionHash() const{
            return hash_;
        }

        User GetUser() const{
            return user_;
        }

        UnclaimedTransactionPtr GetUnclaimedTransaction() const;

        std::string ToString() const{
            std::stringstream stream;
            stream << "Input(" << GetTransactionHash() << "[" << GetOutputIndex() << "]" << ", " << GetUser() << ")";
            return stream.str();
        }

        void operator=(const Input& other){
            hash_ = other.hash_;
            user_ = other.user_;
            index_ = other.index_;
        }

        friend bool operator==(const Input& a, const Input& b){
            return a.hash_ == b.hash_
                && a.index_ == b.index_
                && a.user_ == b.user_;
        }

        friend bool operator!=(const Input& a, const Input& b){
            return a.hash_ != b.hash_
                && a.index_ != b.index_
                && a.user_ != b.user_;
        }

        friend bool operator<(const Input& a, const Input& b){
            if(a.hash_ == b.hash_)
                return a.index_ < b.index_;
            return a.hash_ < b.hash_;
        }

        static inline int64_t
        GetSize(){
            return Hash::GetSize()
                 + sizeof(int32_t)
                 + User::GetSize();
        }
    };

    class InputFileReader : BinaryFileReader{
        friend class TransactionFileReader;
    private:
        InputFileReader(BinaryFileReader* parent): BinaryFileReader(parent){}
    private:
        ~InputFileReader() = default;

        Input Read(){
            Hash hash = ReadHash();
            int32_t index = ReadInt();
            User user = ReadUser();
            return Input(hash, index, user);
        }
    };

    class Output : public Object{
        friend class Transaction;
    private:
        User user_;
        Product product_;
    protected:
        bool Encode(Buffer* buff) const{
            buff->PutUser(user_);
            buff->PutProduct(product_);
            return true;
        }
    public:
        explicit Output():
            user_(),
            product_(){}
        Output(const User& user, const Product& product):
            Object(),
            user_(user),
            product_(product){}
        Output(const std::string& user, const std::string& product):
            Output(User(user), Product(product)){}
        Output(Buffer* buff):
            Object(),
            user_(buff->GetUser()),
            product_(buff->GetProduct()){}
        ~Output(){}

        User GetUser() const{
            return user_;
        }

        Product GetProduct() const{
            return product_;
        }

        std::string ToString() const{
            std::stringstream stream;
            stream << "Output(" << GetUser() << "; " << GetProduct() << ")";
            return stream.str();
        }

        void operator=(const Output& other){
            user_ = other.user_;
            product_ = other.product_;
        }

        friend bool operator==(const Output& a, const Output& b){
            return a.user_ == b.user_
                && a.product_ == b.product_;
        }

        friend bool operator!=(const Output& a, const Output& b){
            return a.user_ != b.user_
                && a.product_ != b.product_;
        }

        friend bool operator<(const Output& a, const Output& b){
            if(a.user_ == b.user_)
                return a.product_ < b.product_;
            return a.user_ < b.user_;
        }

        static inline int64_t
        GetSize(){
            return User::GetSize()
                 + Product::GetSize();
        }
    };

    class OutputFileReader : public BinaryFileReader{
        friend class TransactionFileReader;
    private:
        OutputFileReader(BinaryFileReader* parent): BinaryFileReader(parent){}
    public:
        ~OutputFileReader() = default;

        Output Read(){
            User user = ReadUser();
            Product product = ReadProduct();
            return Output(user, product);
        }
    };

    typedef std::vector<Input> InputList;
    typedef std::vector<Output> OutputList;

    class TransactionInputVisitor;
    class TransactionOutputVisitor;
    class Transaction : public BinaryObject{
        friend class Block;
        friend class TransactionMessage;
    public:
        struct TimestampComparator{
            bool operator()(const TransactionPtr& a, const TransactionPtr& b){
                return a->timestamp_ < b->timestamp_;
            }
        };

        struct IndexComparator{
            bool operator()(const TransactionPtr& a, const TransactionPtr& b){
                return a->index_ < b->index_;
            }
        };

        static const int64_t kMaxNumberOfInputs;
        static const int64_t kMaxNumberOfOutputs;
    private:
        Timestamp timestamp_;
        int64_t index_;
        InputList inputs_;
        OutputList outputs_;
        std::string signature_;
    protected:
        bool Write(Buffer* buff) const{
            buff->PutLong(timestamp_);
            buff->PutLong(index_);

            int64_t idx;
            buff->PutLong(GetNumberOfInputs());
            for(idx = 0;
                idx < GetNumberOfInputs();
                idx++){
                inputs_[idx].Encode(buff);
            }
            buff->PutLong(GetNumberOfOutputs());
            for(idx = 0;
                idx < GetNumberOfOutputs();
                idx++){
                outputs_[idx].Encode(buff);
            }
            //TODO: serialize transaction signature
            return true;
        }

        int64_t GetBufferSize() const{
            int64_t size = 0;
            size += sizeof(Timestamp); // timestamp_
            size += sizeof(int64_t); // index_
            size += sizeof(int64_t); // num_inputs_
            size += GetNumberOfInputs() * Input::GetSize(); // inputs_
            size += sizeof(int64_t); // num_outputs
            size += GetNumberOfOutputs() * Output::GetSize(); // outputs_
            return size;
        }
    public:
        Transaction(int64_t index, const InputList& inputs, const OutputList& outputs, Timestamp timestamp=GetCurrentTimestamp()):
            BinaryObject(),
            timestamp_(timestamp),
            index_(index),
            inputs_(inputs),
            outputs_(outputs),
            signature_(){}
        ~Transaction() = default;

        Timestamp GetTimestamp() const{
            return timestamp_;
        }

        int64_t GetIndex() const{
            return index_;
        }

        int64_t GetNumberOfInputs() const{
            return inputs_.size();
        }

        InputList& inputs(){
            return inputs_;
        }

        InputList inputs() const{
            return inputs_;
        }

        OutputList& outputs(){
            return outputs_;
        }

        OutputList outputs() const{
            return outputs_;
        }

        InputList::iterator inputs_begin(){
            return inputs_.begin();
        }

        InputList::const_iterator inputs_begin() const{
            return inputs_.begin();
        }

        InputList::iterator inputs_end(){
            return inputs_.end();
        }

        InputList::const_iterator inputs_end() const{
            return inputs_.end();
        }

        int64_t GetNumberOfOutputs() const{
            return outputs_.size();
        }

        OutputList::iterator outputs_begin(){
            return outputs_.begin();
        }

        OutputList::const_iterator outputs_begin() const{
            return outputs_.begin();
        }

        OutputList::iterator outputs_end(){
            return outputs_.end();
        }

        OutputList::const_iterator outputs_end() const{
            return outputs_.end();
        }

        std::string GetSignature() const{
            return signature_;
        }

        bool IsSigned() const{
            return !signature_.empty();
        }

        bool IsCoinbase() const{
            return GetIndex() == 0;
        }

        bool Sign();
        bool VisitInputs(TransactionInputVisitor* vis) const;
        bool VisitOutputs(TransactionOutputVisitor* vis) const;

        std::string ToString() const{
            std::stringstream stream;
            stream << "Transaction(#" << GetIndex() << "," << GetNumberOfInputs() << " Inputs, " << GetNumberOfOutputs() << " Outputs)";
            return stream.str();
        }

        void operator=(const Transaction& other){
            timestamp_ = other.timestamp_;
            index_ = other.index_;
            inputs_ = other.inputs_;
            outputs_ = other.outputs_;
            //TODO: copy transaction signature
        }

        bool operator==(const Transaction& other){
            return timestamp_ == other.timestamp_
                && index_ == other.index_
                && inputs_ == other.inputs_
                && outputs_ == other.outputs_;
            //TODO: compare transaction signature
        }

        bool operator!=(const Transaction& other){
            return timestamp_ != other.timestamp_
                && index_ != other.index_
                && inputs_ != other.inputs_
                && outputs_ != other.outputs_;
            //TODO: compare transaction signature
        }

        static TransactionPtr NewInstance(Buffer* buff){
            Timestamp timestamp = buff->GetLong();
            int64_t index = buff->GetLong();

            InputList inputs;

            int64_t idx;
            int64_t num_inputs = buff->GetLong();
            for(idx = 0; idx < num_inputs; idx++)
                inputs.push_back(Input(buff));

            OutputList outputs;
            int64_t num_outputs = buff->GetLong();
            for(idx = 0; idx < num_outputs; idx++)
                outputs.push_back(Output(buff));
            return std::make_shared<Transaction>(index, inputs, outputs, timestamp);
        }

        static TransactionPtr NewInstance(std::fstream& fd, size_t size);
        static inline TransactionPtr NewInstance(const std::string& filename){
            std::fstream fd(filename, std::ios::in|std::ios::binary);
            return NewInstance(fd, GetFilesize(filename));
        }
    };

    class TransactionFileWriter : BinaryFileWriter{
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
        TransactionFileWriter(const std::string& filename): BinaryFileWriter(filename){}
        TransactionFileWriter(BinaryFileWriter* parent): BinaryFileWriter(parent){}
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

    class TransactionFileReader : BinaryFileReader{
    private:
        InputFileReader input_reader_;
        OutputFileReader output_reader_;

        inline Input
        ReadNextInput(){
            return input_reader_.Read();
        }

        inline Output
        ReadNextOutput(){
            return output_reader_.Read();
        }
    public:
        TransactionFileReader(const std::string& filename):
            BinaryFileReader(filename),
            input_reader_(this),
            output_reader_(this){}
        TransactionFileReader(BinaryFileReader* parent):
            BinaryFileReader(parent),
            input_reader_(this),
            output_reader_(this){}
        ~TransactionFileReader() = default;

        TransactionPtr Read(){
            Timestamp timestamp = ReadLong();
            int64_t index = ReadLong();

            InputList inputs;
            int64_t num_inputs = ReadLong();
            for(int64_t idx = 0; idx < num_inputs; idx++)
                inputs.push_back(ReadNextInput());

            OutputList outputs;
            int64_t num_outputs = ReadLong();
            for(int64_t idx = 0; idx < num_outputs; idx++)
                outputs.push_back(ReadNextOutput());

            return std::make_shared<Transaction>(index, inputs, outputs, timestamp);
        }
    };

    typedef std::vector<TransactionPtr> TransactionList;

    class TransactionInputVisitor{
    protected:
        TransactionInputVisitor() = default;
    public:
        virtual ~TransactionInputVisitor() = default;
        virtual bool Visit(const Input& input) = 0;
    };

    class TransactionOutputVisitor{
    protected:
        TransactionOutputVisitor() = default;
    public:
        virtual ~TransactionOutputVisitor() = default;
        virtual bool Visit(const Output& output) = 0;
    };

    class TransactionPrinter : public Printer, public TransactionInputVisitor, public TransactionOutputVisitor{
    public:
        TransactionPrinter(const google::LogSeverity& severity=google::INFO, const long& flags=Printer::kFlagNone):
            Printer(severity, flags){}
        TransactionPrinter(Printer* parent):
            Printer(parent){}
        ~TransactionPrinter() = default;

        bool Visit(const Input& input){
            if(!IsDetailed())
                return true;
            LOG_AT_LEVEL(GetSeverity()) << "Input(" << input.GetTransactionHash() << "[" << input.GetOutputIndex() << "]";
            return true;
        }

        bool Visit(const Output& output){
            if(!IsDetailed())
                return true;
            LOG_AT_LEVEL(GetSeverity()) << "Output(" << output.GetUser() << ", " << output.GetProduct() << ")";
            return true;
        }
    };
}

#endif //TOKEN_TRANSACTION_H