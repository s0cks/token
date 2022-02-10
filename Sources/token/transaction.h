#ifndef TOKEN_TRANSACTION_H
#define TOKEN_TRANSACTION_H

#include "token/timestamp.h"
#include "token/input.h"
#include "token/output.h"

namespace token{
 class Transaction : public SerializableObject{
  protected:
   Timestamp timestamp_;

   Input* inputs_;
   uint64_t num_inputs_;

   Output* outputs_;
   uint64_t num_outputs_;
  public:
   Transaction():
    timestamp_(Clock::now()),
    inputs_(nullptr),
    num_inputs_(0),
    outputs_(nullptr),
    num_outputs_(0){
   }
   /**
    * Non-Copy constructor
    * @param inputs The inputs for the transaction
    * @param num_inputs The number of inputs for the transaction
    * @param outputs The outputs for the transaction
    * @param num_outputs The number of outputs for the transaction
    */
   Transaction(const Timestamp& timestamp, const Input* inputs, uint64_t num_inputs, const Output* outputs, uint64_t num_outputs):
    timestamp_(timestamp),
    inputs_(new Input[num_inputs]),
    num_inputs_(num_inputs),
    outputs_(new Output[num_outputs]),
    num_outputs_(num_outputs){
     std::copy(inputs, inputs + num_inputs, inputs_begin());
     std::copy(outputs, outputs + num_outputs, outputs_begin());
   }
   Transaction(const Input* inputs, uint64_t num_inputs, const Output* outputs, uint64_t num_outputs):
    Transaction(Clock::now(), inputs, num_inputs, outputs, num_outputs){
   }
   explicit Transaction(const BufferPtr& data):
    timestamp_(data->GetTimestamp()),
    inputs_(nullptr),
    num_inputs_(0),
    outputs_(nullptr),
    num_outputs_(0){
     if(!data->GetList(&num_inputs_, &inputs_)){
       LOG(FATAL) << "cannot parse inputs_ from buffer.";
       return;
     }

     if(!data->GetList(&num_outputs_, &outputs_)){
       LOG(FATAL) << "cannot parse outputs_ from buffer.";
       return;
     }
   }
   Transaction(const Transaction& rhs):
    timestamp_(rhs.timestamp()),
    inputs_(new Input[rhs.GetNumberOfInputs()]),
    num_inputs_(rhs.GetNumberOfInputs()),
    outputs_(new Output[rhs.GetNumberOfOutputs()]),
    num_outputs_(rhs.GetNumberOfOutputs()){
     std::copy(rhs.inputs_begin(), rhs.inputs_end(), inputs_begin());
     std::copy(rhs.outputs_begin(), rhs.outputs_end(), outputs_begin());
   }
   ~Transaction() override{
     delete[] inputs_;
     delete[] outputs_;
   }

   Type type() const override{
     return Type::kTransaction;
   }

   Timestamp timestamp() const{
     return timestamp_;
   }

   Input* inputs(uint64_t idx) const{
     return &inputs_[idx];
   }

   uint64_t GetNumberOfInputs() const{
     return num_inputs_;
   }

   Input* inputs_begin() const{
     return &inputs_[0];
   }

   Input* inputs_end() const{
     return &inputs_[num_inputs_];
   }

   Output* outputs(uint64_t idx) const{
     return &outputs_[idx];
   }

   uint64_t GetNumberOfOutputs() const{
     return num_outputs_;
   }

   Output* outputs_begin() const{
     return &outputs_[0];
   }

   Output* outputs_end() const{
     return &outputs_[num_outputs_];
   }

   void VisitInputs(InputVisitor* vis) {
     auto fn = std::bind(&InputVisitor::Visit, vis, std::placeholders::_1);
     std::for_each(inputs_begin(), inputs_end(), fn);
   }

   void VisitOutputs(OutputVisitor* vis){
     auto fn = std::bind(&OutputVisitor::Visit, vis, std::placeholders::_1);
     std::for_each(outputs_begin(), outputs_end(), fn);
   }

   uint64_t GetBufferSize() const override{
     auto size = 0;
     size += sizeof(RawTimestamp);

     size += sizeof(uint64_t);
     size += (GetNumberOfInputs() * Input::kSize);

     size += sizeof(uint64_t);
     size += (GetNumberOfOutputs() + Output::kSize);
     return size;
   }

   bool WriteTo(const BufferPtr& data) const override{
     if(!data->PutTimestamp(timestamp())){
       LOG(FATAL) << "cannot put timestamp_ into buffer.";
       return false;
     }

     if(!data->PutList(inputs_, GetNumberOfInputs())){
       LOG(FATAL) << "cannot put inputs_ into buffer.";
       return false;
     }

     if(!data->PutList(outputs_, GetNumberOfOutputs())){
       LOG(FATAL) << "cannot put outputs into buffer.";
       return false;
     }
     return true;
   }

   Transaction& operator=(const Transaction& rhs){
     if(&rhs == this)
       return *this;
     timestamp_ = rhs.timestamp();
     num_inputs_ = rhs.GetNumberOfInputs();

     if(inputs_ != nullptr)
       delete[] inputs_;
     inputs_ = new Input[rhs.GetNumberOfInputs()];
     std::copy(rhs.inputs_begin(), rhs.inputs_end(), inputs_begin());

     if(outputs_ != nullptr)
       delete[] outputs_;
     outputs_ = new Output[rhs.GetNumberOfOutputs()];
     std::copy(rhs.outputs_begin(), rhs.outputs_end(), outputs_begin());
     return *this;
   }

   friend bool operator==(const Transaction& lhs, const Transaction& rhs){
     return lhs.timestamp() == rhs.timestamp()
         && lhs.GetNumberOfInputs() == rhs.GetNumberOfInputs()
         && std::equal(lhs.inputs_begin(), lhs.inputs_end(), rhs.inputs_begin())
         && lhs.GetNumberOfOutputs() == rhs.GetNumberOfOutputs()
         && std::equal(lhs.outputs_begin(), lhs.outputs_end(), rhs.outputs_begin());
   }

   friend bool operator!=(const Transaction& lhs, const Transaction& rhs){
     return !operator==(lhs, rhs);
   }

   friend bool operator<(const Transaction& lhs, const Transaction& rhs){
     return lhs.timestamp() < rhs.timestamp();
   }

   friend bool operator>(const Transaction& lhs, const Transaction& rhs){
     return lhs.timestamp() > rhs.timestamp();
   }
 };

 class IndexedTransaction : public Transaction{
  protected:
   uint64_t index_;
  public:
   IndexedTransaction():
    Transaction(),
    index_(){
   }
   IndexedTransaction(uint64_t index, const Timestamp& timestamp, const Input* inputs, uint64_t num_inputs, const Output* outputs, uint64_t num_outputs):
    Transaction(timestamp, inputs, num_inputs, outputs, num_outputs),
    index_(index){
   }
   IndexedTransaction(uint64_t index, const Transaction& tx):
    Transaction(tx),
    index_(index){
   }
   IndexedTransaction(uint64_t index, const Input* inputs, uint64_t num_inputs, const Output* outputs, uint64_t num_outputs):
    Transaction(inputs, num_inputs, outputs, num_outputs),
    index_(index){
   }
   explicit IndexedTransaction(const BufferPtr& data):
    Transaction(),
    index_(data->GetUnsignedLong()){
     timestamp_ = data->GetTimestamp();
     if(!data->GetList(&num_inputs_, &inputs_))
       LOG(FATAL) << "cannot parse inputs_ from buffer.";
     if(!data->GetList(&num_outputs_, &outputs_))
       LOG(FATAL) << "cannot parse outputs_ from buffer.";
   }
   IndexedTransaction(const IndexedTransaction& rhs):
    Transaction(rhs),
    index_(rhs.index()){
   }
   ~IndexedTransaction() override = default;

   Type type() const override{
     return Type::kIndexedTransaction;
   }

   uint64_t index() const{
     return index_;
   }

   uint64_t GetBufferSize() const override{
     auto size = 0;
     size += Transaction::GetBufferSize();
     size += sizeof(uint64_t);
     return size;
   }

   bool WriteTo(const BufferPtr& data) const override{
     if(!data->PutUnsignedLong(index()))
       return false;
     return Transaction::WriteTo(data);
   }

   IndexedTransaction& operator=(const IndexedTransaction& rhs){
     if(&rhs == this)
       return *this;
     Transaction::operator=(rhs);
     index_ = rhs.index();
     return *this;
   }

   friend std::ostream& operator<<(std::ostream& stream, const IndexedTransaction& val){
     return stream << "IndexedTransaction(index=" << val.index() << ", timestamp=" << FormatTimestampReadable(val.timestamp()) << ")";
   }

   friend bool operator==(const IndexedTransaction& lhs, const IndexedTransaction& rhs){
     return lhs.index() == rhs.index()
         && lhs.timestamp() == rhs.timestamp()
         && lhs.GetNumberOfInputs() == rhs.GetNumberOfInputs()
         && std::equal(lhs.inputs_begin(), lhs.inputs_end(), rhs.inputs_begin())
         && lhs.GetNumberOfOutputs() == rhs.GetNumberOfOutputs()
         && std::equal(lhs.outputs_begin(), lhs.outputs_end(), rhs.outputs_begin());
   }

   friend bool operator!=(const IndexedTransaction& lhs, const IndexedTransaction& rhs){
     return !operator==(lhs, rhs);
   }

   friend bool operator<(const IndexedTransaction& lhs, const IndexedTransaction& rhs){
     return lhs.index() < rhs.index();
   }

   friend bool operator>(const IndexedTransaction& lhs, const IndexedTransaction& rhs){
     return lhs.index() > rhs.index();
   }
 };
}

#endif //TOKEN_TRANSACTION_H