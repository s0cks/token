#include "user.h"
#include "buffer.h"
#include "transaction_input.h"

namespace token{
  Input::Input(const BufferPtr& buff):
    Input(buff->GetReference(), buff->GetUser()){}

  Input Input::ParseFrom(const BufferPtr& buff){
    return Input(buff->GetReference(), buff->GetUser());
  }

  std::string Input::ToString() const{
    std::stringstream stream;
    stream << "Input(";
    stream << "reference=" << GetReference() << ",";
    stream << "user=" << GetUser();
    return stream.str();
  }

  bool Input::Write(const BufferPtr& buff) const{
    return buff->PutReference(reference_)
        && buff->PutUser(user_);
  }
}