#ifndef TOKEN_TRIE_H
#define TOKEN_TRIE_H

#include <string>
#include <cstdint>

#include <hash.h>

namespace Token{
  template<typename K, typename V, int64_t kAlphabetSize>
  class Trie;

  template<typename K, typename V, int64_t kAlphabetSize>
  class TrieNode;

  template<typename T>
  class TrieKey{
   private:
    T value_;
   public:
    TrieKey():
      value_(){}
    TrieKey(const T& value):
      value_(value){}
    ~TrieKey() = default;

    friend bool operator==(const TrieKey<T>& a, const TrieKey<T>& b){
      return a.value_ == b.value_;
    }

    friend bool operator!=(const TrieKey<T>& a, const TrieKey<T>& b){
      return a.value_ != b.value_;
    }

    friend bool operator<(const TrieKey<T>& a, const TrieKey<T>& b){
      return a.value_ < b.value_;
    }
  };

  template<typename K, typename V, int64_t kAlphabetSize>
  class TrieNode{
    friend class Trie<K, V, kAlphabetSize>;
   private:
    std::shared_ptr<TrieNode<K, V, kAlphabetSize>> parent_;
    std::shared_ptr<TrieNode<K, V, kAlphabetSize>> children_[kAlphabetSize];
    TrieKey<K> key_;
    V value_;
    bool is_epsilon_;

    void SetEpsilon(){
      is_epsilon_ = true;
    }

    void ClearEpsilon(){
      is_epsilon_ = false;
    }

    void SetValue(const V& value){
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
        children_[idx] = std::shared_ptr<TrieNode<K, V, kAlphabetSize>>(nullptr);
    }
    TrieNode(const K& key, V value):
      parent_(nullptr),
      children_(),
      key_(key),
      value_(value),
      is_epsilon_(false){
      for(size_t idx = 0; idx < kAlphabetSize; idx++)
        children_[idx] = std::shared_ptr<TrieNode<K, V, kAlphabetSize>>(nullptr);
    }
    ~TrieNode() = default;

    std::shared_ptr<TrieNode<K, V, kAlphabetSize>> GetParent() const{
      return parent_;
    }

    std::shared_ptr<TrieNode<K, V, kAlphabetSize>> GetChild(intptr_t idx) const{
      return children_[idx];
    }

    TrieKey<K> GetKey() const{
      return key_;
    }

    V GetValue() const{
      return value_;
    }

    bool HasValue() const{
      return value_ != nullptr;
    }

    bool IsEpsilon() const{
      return is_epsilon_;
    }
  };

  template<typename K, typename V, int64_t kAlphabetSize>
  class Trie{
   private:
    std::unique_ptr<TrieNode<K, V, kAlphabetSize>> root_;

    void Insert(const std::shared_ptr<TrieNode<K, V, kAlphabetSize>>& node, const K& key, const V& value){
      std::shared_ptr<TrieNode<K, V, kAlphabetSize>> curr = node;
      for(size_t idx = 0; idx < key.size(); idx++){
        char next = tolower(key[idx]);

        std::shared_ptr<TrieNode<K, V, kAlphabetSize>>& n = curr->children_[(int) next];
        if(!n){
          curr->children_[(int) next] = n = std::make_shared<TrieNode<K, V, kAlphabetSize>>(key, value);
        }
        curr = n;
      }

      curr->SetValue(value);
      curr->SetEpsilon();
    }

    std::shared_ptr<TrieNode<K, V, kAlphabetSize>> Search(const std::shared_ptr<TrieNode<K, V, kAlphabetSize>>& node,
                                                          const K& key){
      std::shared_ptr<TrieNode<K, V, kAlphabetSize>> n = node;

      const char* word = key.data();
      while((*word) != '\0'){
        if(n->children_[tolower((*word))].get()){
          n = n->children_[tolower((*word))];
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
      root_(std::unique_ptr<TrieNode<K, V, kAlphabetSize>>()){}
    ~Trie() = default;

    std::shared_ptr<TrieNode<K, V, kAlphabetSize>> GetRoot() const{
      return std::shared_ptr<TrieNode<K, V, kAlphabetSize>>(root_.get());
    }

    void Insert(const K& key, const V& value){
      Insert(GetRoot(), key, value);
    }

    bool Contains(const K& key){
      std::shared_ptr<TrieNode<K, V, kAlphabetSize>> node = Search(GetRoot(), key);
      return !node;
    }

    V Search(const K& key){
      std::shared_ptr<TrieNode<K, V, kAlphabetSize>> node = Search(GetRoot(), key);
      return node != nullptr
             ? node->GetValue()
             : nullptr;
    }
  };
}

#endif //TOKEN_TRIE_H