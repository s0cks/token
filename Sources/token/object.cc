#include "token/object.h"

namespace token{
 bool SerializableObject::WriteTo(const std::string& filename) const{
   auto data = ToBuffer();
   FILE* file;
   if((file = fopen(filename.data(), "wb")) == nullptr){
     LOG(FATAL) << "couldn't open file " << filename << " for writing: " << strerror(errno);
     return false;
   }
   return data->WriteTo(file);
 }
}