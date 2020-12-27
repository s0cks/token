#ifndef TOKEN_JSON_H
#define TOKEN_JSON_H

#include <rapidjson/writer.h>
#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>

#include "hash.h"
#include "block.h"

namespace Token{
    static inline void
    ToJson(const HashList& hashes, rapidjson::Document& doc){
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        doc.SetArray();
        for(auto& it : hashes){
            std::string hex = it.HexString();
            rapidjson::Value val(hex.data(), 64, alloc);
            doc.PushBack(val, alloc);
        }
    }

    static inline void
    SetField(rapidjson::Document& doc, const char* name, const Hash& hash){
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value key(name, alloc);

        std::string hex = hash.HexString();
        rapidjson::Value val(hex.data(), 64, alloc);
        doc.AddMember(key, val, alloc);
    }

    static inline void
    SetField(rapidjson::Document& doc, const char* name, const int64_t& value){
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value key(name, alloc);
        rapidjson::Value val(value);
        doc.AddMember(key, val, alloc);
    }

    static inline void
    SetField(rapidjson::Document& doc, const char* name, const User& value){
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value key(name, alloc);

        rapidjson::Value val((char*)value.data(), User::GetSize(), alloc);
        doc.AddMember(key, val, alloc);
    }

    static inline void
    SetField(rapidjson::Document& doc, const char* name, const Product& value){
        rapidjson::Document::AllocatorType& alloc = doc.GetAllocator();
        rapidjson::Value key(name, alloc);

        rapidjson::Value val((char*)value.data(), Product::GetSize(), alloc);
        doc.AddMember(key, val, alloc);
    }

    static inline void
    ToJson(const UnclaimedTransactionPtr& utxo, rapidjson::Document& doc){
        doc.SetObject();
        SetField(doc, "TransactionHash", utxo->GetTransaction());
        SetField(doc, "OutputIndex", utxo->GetIndex());
        SetField(doc, "User", utxo->GetUser());
        SetField(doc, "Product", utxo->GetProduct());
    }

    static inline void
    ToJson(const BlockPtr& blk, rapidjson::Document& doc){
        doc.SetObject();

        SetField(doc, "Timestamp", blk->GetTimestamp());
        SetField(doc, "Height", blk->GetHeight());
        SetField(doc, "PreviousHash", blk->GetPreviousHash());
        SetField(doc, "MerkleRoot", blk->GetMerkleRoot());
        SetField(doc, "Hash", blk->GetHash());
    }
}

#endif //TOKEN_JSON_H