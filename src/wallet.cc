#include "wallet.h"
#include "buffer.h"

namespace token{
  int64_t GetBufferSize(const Wallet& wallet){
    int64_t size = 0;
    size += sizeof(int64_t);
    size += (wallet.size() * Hash::kSize);
    return size;
  }

  bool Encode(const BufferPtr& buff, const Wallet& wallet){
    if(!buff->PutLong(wallet.size()))
      return false;
    return std::all_of(wallet.begin(), wallet.end(), [&buff](const Hash& hash){
      return buff->PutHash(hash);
    });
  }

  bool Decode(const std::string& data, Wallet& wallet){
    Buffer buff(data);
    int64_t len = buff.GetLong();
    for(int64_t idx = 0; idx < len; idx++)
      if(!wallet.insert(buff.GetHash()).second)
        return false;
    return true;
  }

  bool Decode(const BufferPtr& buff, Wallet& wallet){
    int64_t size = buff->GetLong();
    for(int64_t idx = 0; idx < size; idx++)
      if(!wallet.insert(buff->GetHash()).second)
        return false;
    return true;
  }

  void ToJson(const Wallet& hashes, Json::String& sb){
    Json::Writer writer(sb);
    writer.StartArray();
    for(auto& it : hashes){
      std::string hash = it.HexString();
      writer.String(hash.data(), Hash::GetSize());
    }
    writer.EndArray();
  }
}