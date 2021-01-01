#ifndef TOKEN_JSON_CONVERSION_H
#define TOKEN_JSON_CONVERSION_H

#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

#include "hash.h"
#include "block.h"

namespace Token{
    typedef rapidjson::StringBuffer JsonString;
    typedef rapidjson::Writer<JsonString> JsonWriter;

    void ToJson(const HashList& hashes, JsonString& json);
    void ToJson(const BlockPtr& blk, JsonString& json);
    void ToJson(const TransactionPtr& tx, JsonString& json);
    void ToJson(const UnclaimedTransactionPtr& utxo, JsonString& json);
}

#endif //TOKEN_JSON_CONVERSION_H