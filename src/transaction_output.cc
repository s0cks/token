#include "buffer.h"
#include "transaction_output.h"

namespace token{
  Output::Output(const BufferPtr& buff):
    Output(buff->GetUser(), buff->GetProduct()){}

  Output Output::ParseFrom(const BufferPtr& buff){
    return Output(buff->GetUser(), buff->GetProduct());
  }

  std::string Output::ToString() const{
    std::stringstream ss;
    ss << "Output(";
    ss << "user=" << GetUser() << ",";
    ss << "product=" << GetProduct();
    ss << ")";
    return ss.str();
  }

  bool Output::Write(const BufferPtr& buff) const{
    return buff->PutUser(user_)
        && buff->PutProduct(product_);
  }
}