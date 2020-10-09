#ifndef TOKEN_TRIE_H
#define TOKEN_TRIE_H

#include "common.h"
#include "block.h"

namespace Token{
    template<typename Key, typename Value>
    class TrieNode{
        template<typename TKey, typename TValue>
        friend class Trie;
    public:
        static const intptr_t kAlphabetSize = 64;
    private:
        TrieNode<Key, Value>* parent_;
        TrieNode<Key, Value>* children_[kAlphabetSize];
        Key key_;
        Value* value_;
        bool is_epsilon_;

        void SetEpsilon(){
            is_epsilon_ = true;
        }

        void ClearEpsilon(){
            is_epsilon_ = false;
        }

        void SetValue(const Handle<Value>& value){
            value_ = value;
        }
    public:
        TrieNode():
            parent_(nullptr),
            children_(),
            key_(uint256_t::Null()),
            value_(nullptr),
            is_epsilon_(false){
            for(size_t idx = 0; idx < kAlphabetSize; idx++)
                children_[idx] = nullptr;
        }
        ~TrieNode() = default;

        TrieNode* GetParent() const{
            return parent_;
        }

        TrieNode* GetChild(intptr_t idx) const{
            return children_[idx];
        }

        Key GetKey() const{
            return key_;
        }

        Handle<Value> GetValue() const{
            return value_;
        }

        bool HasValue() const{
            return value_ != nullptr;
        }

        bool IsEpsilon() const{
            return is_epsilon_;
        }
    };

    template<typename Key, typename Value>
    class Trie{
    private:
        TrieNode<Key, Value>* root_;

        void Insert(TrieNode<Key, Value>* node, const Key& key, const Handle<Value>& value){
            TrieNode<Key, Value>* curr = node;
            std::string prefix = key.HexString();
            for(size_t idx = 0; idx < prefix.size(); idx++){
                char next = tolower(prefix[idx]);
                TrieNode<Key, Value>* n = curr->children_[next];
                if(!n){
                    n = new TrieNode<Key, Value>();
                    curr->children_[next] = n;
                }
                curr = n;
            }

            curr->SetValue(value);
            curr->SetEpsilon();
        }

        TrieNode<Key, Value>* Search(TrieNode<Key, Value>* node, const Key& key){
            std::string prefix = key.HexString();
            const char* word = prefix.data();

            while((*word) != '\0'){
                if(node->children_[tolower((*word))]){
                    node = node->children_[tolower((*word))];
                    word++;
                } else{
                    break;
                }
            }

            return (*word) == '\0'
                 ? node
                 : nullptr;
        }
    public:
        Trie(): root_(new TrieNode<Key, Value>()){}
        ~Trie() = default;

        TrieNode<Key, Value>* GetRoot() const{
            return root_;
        }

        void Insert(const Key& key, const Handle<Value>& value){
            Insert(GetRoot(), key, value);
        }

        bool Contains(const Key& key){
            TrieNode<Key, Value>* node = Search(GetRoot(), key);
            return node != nullptr;
        }

        Handle<Value> Search(const Key& key){
            TrieNode<Key, Value>* node = Search(GetRoot(), key);
            return node != nullptr
                 ? node->GetValue()
                 : nullptr;
        }
    };
}

#endif //TOKEN_TRIE_H