#ifndef TOKEN_TRIE_H
#define TOKEN_TRIE_H

#include <string>
#include <hash.h>
#include <cstdint>

namespace token{
  template<class K, class V, int64_t kAlphabetSize>
  class Trie;

  template<class K, class V, int64_t kAlphabetSize>
  class TrieNode{
   private:
    TrieNode<K, V, kAlphabetSize>* parent_;
    TrieNode<K, V, kAlphabetSize>* children_[kAlphabetSize];
    K key_;
    V value_;
    bool epsilon_;

    void ClearEpsilon(){
      epsilon_ = false;
    }

    void SetEpsilon(){
      epsilon_ = true;
    }
   public:
    TrieNode():
      parent_(nullptr),
      children_(),
      key_(),
      value_(),
      epsilon_(false){
      for(int idx = 0; idx < kAlphabetSize; idx++)
        children_[idx] = nullptr;
    }
    TrieNode(const K& key, const V& value):
      parent_(nullptr),
      children_(),
      key_(key),
      value_(value),
      epsilon_(false){
      for(int idx = 0; idx < kAlphabetSize; idx++)
        children_[idx] = nullptr;
    }
    virtual ~TrieNode() = default;

    TrieNode<K, V, kAlphabetSize>* GetParent() const{
      return parent_;
    }

    TrieNode<K, V, kAlphabetSize>* GetChild(int pos) const{
      if(pos < 0 || pos > kAlphabetSize)
        return nullptr;
      return children_[pos];
    }

    K& GetKey(){
      return key_;
    }

    K GetKey() const{
      return key_;
    }

    V& GetValue(){
      return value_;
    }

    V GetValue() const{
      return value_;
    }

    bool HasValue() const{
      return value_ != nullptr;
    }

    bool IsEpsilon() const{
      return epsilon_;
    }

    bool HasChild(int pos) const{
      if(pos < 0 || pos > kAlphabetSize)
        return false;
      return children_[pos] != nullptr;
    }

    bool SetChild(int pos, TrieNode<K, V, kAlphabetSize>* node){
      if(pos < 0 || pos > kAlphabetSize)
        return false;
      children_[pos] = node;
      return true;
    }
  };

  template<typename K, typename V, int64_t kAlphabetSize>
  class Trie{
   private:
    TrieNode<K, V, kAlphabetSize>* root_;

    void Insert(TrieNode<K, V, kAlphabetSize>* node, const K& key, const V& value){
      TrieNode<K, V, kAlphabetSize>* curr = node;
      for(int idx = 0; idx < key.size(); idx++){
        unsigned char next = tolower(key[idx]);

        TrieNode<K, V, kAlphabetSize>* child = curr->children_[next];
        if(!child)
          child = curr->children_[(int)next] = new TrieNode<K, V, kAlphabetSize>(key, value);
        curr = child;
      }

      curr->SetValue(value);
      curr->SetEpsilon();
    }

    TrieNode<K, V, kAlphabetSize>* Search(TrieNode<K, V, kAlphabetSize> node, const K& key){
      TrieNode<K, V, kAlphabetSize>* n = node;

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
      root_(new TrieNode<K, V, kAlphabetSize>()){}
    ~Trie(){
      if(root_)
        delete root_;
    }

    TrieNode<K, V, kAlphabetSize>* GetRoot() const{
      return root_;
    }

    void Insert(const K& key, const V& value){
      Insert(GetRoot(), key, value);
    }

    bool Contains(const K& key){
      std::shared_ptr<TrieNode<K, V, kAlphabetSize>> node = Search(GetRoot(), key);
      return !node;
    }

    V Search(const K& key){
      TrieNode<K, V, kAlphabetSize>* node = Search(GetRoot(), key);
      return node != nullptr
             ? node->GetValue()
             : nullptr;
    }
  };
}

#endif //TOKEN_TRIE_H