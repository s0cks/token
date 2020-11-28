#ifndef TOKEN_TRIE_H
#define TOKEN_TRIE_H

#include <string>
#include <cstdint>

#include <hash.h>

namespace Token{
    template<typename KeyType>
    class TrieKey{
    private:
        KeyType value_;
    public:
        TrieKey():
            value_(){}
        TrieKey(const KeyType& value):
            value_(value){}
        ~TrieKey() = default;

        friend bool operator==(const TrieKey<KeyType>& a, const TrieKey<KeyType>& b){
            return a.value_ == b.value_;
        }

        friend bool operator!=(const TrieKey<KeyType>& a, const TrieKey<KeyType>& b){
            return a.value_ != b.value_;
        }

        friend bool operator<(const TrieKey<KeyType>& a, const TrieKey<KeyType>& b){
            return a.value_ < b.value_;
        }
    };

    template<typename Key, typename Value, int64_t kAlphabetSize>
    class TrieNode{
        template<typename TKey, typename TValue, int64_t tkAlphabetSize>
        friend class Trie;
    private:
        TrieNode<Key, Value, kAlphabetSize>* parent_;
        TrieNode<Key, Value, kAlphabetSize>* children_[kAlphabetSize];
        TrieKey<Key> key_;
        Value value_;
        bool is_epsilon_;

        void SetEpsilon(){
            is_epsilon_ = true;
        }

        void ClearEpsilon(){
            is_epsilon_ = false;
        }

        void SetValue(Value value){
            value_ = value;
        }
    public:
        TrieNode():
            parent_(nullptr),
            children_(),
            key_(),
            value_(),
            is_epsilon_(false){
            for(size_t idx = 0; idx < kAlphabetSize; idx++)
                children_[idx] = nullptr;
        }
        TrieNode(const Key& key, Value value):
            parent_(nullptr),
            children_(),
            key_(TrieKey<Key>(key)),
            value_(value),
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

        TrieKey<Key> GetKey() const{
            return key_;
        }

        Value GetValue() const{
            return value_;
        }

        bool HasValue() const{
            return value_ != nullptr;
        }

        bool IsEpsilon() const{
            return is_epsilon_;
        }
    };

    template<typename Key, typename Value, int64_t kAlphabetSize>
    class Trie{
    private:
        TrieNode<Key, Value, kAlphabetSize>* root_;

        void Insert(TrieNode<Key, Value, kAlphabetSize>* node, const Key& key, Value value){
            TrieNode<Key, Value, kAlphabetSize>* curr = node;
            for(size_t idx = 0; idx < key.size(); idx++){
                char next = tolower(key[idx]);
                TrieNode<Key, Value, kAlphabetSize>* n = curr->children_[(int)next];
                if(!n){
                    n = new TrieNode<Key, Value, kAlphabetSize>(key, value);
                    curr->children_[(int)next] = n;
                }
                curr = n;
            }

            curr->SetValue(value);
            curr->SetEpsilon();
        }

        TrieNode<Key, Value, kAlphabetSize>* Search(TrieNode<Key, Value, kAlphabetSize>* node, const Key& key){
            const char* word = key.data();

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
        Trie():
            root_(new TrieNode<Key, Value, kAlphabetSize>()){}
        ~Trie() = default;

        TrieNode<Key, Value, kAlphabetSize>* GetRoot() const{
            return root_;
        }

        void Insert(const Key& key, Value value){
            Insert(GetRoot(), key, value);
        }

        bool Contains(const Key& key){
            TrieNode<Key, Value, kAlphabetSize>* node = Search(GetRoot(), key);
            return node != nullptr;
        }

        Value Search(const Key& key){
            TrieNode<Key, Value, kAlphabetSize>* node = Search(GetRoot(), key);
            return node != nullptr
                 ? node->GetValue()
                 : nullptr;
        }
    };
}

#endif //TOKEN_TRIE_H