#ifndef TOKEN_ATOMIC_LINKED_LIST_H
#define TOKEN_ATOMIC_LINKED_LIST_H

#include <atomic>

namespace Token{
  template<typename T>
  class LinkedListNode{
    template<typename U>
    friend
    class AtomicLinkedList;
   private:
    LinkedListNode *prev_;
    LinkedListNode *next_;
    T value_;
   public:
    LinkedListNode(const T &value):
        value_(value){}
    ~LinkedListNode() = default;
  };

  template<typename T>
  class AtomicLinkedListNode{
    //TODO: fixme
    template<typename U>
    friend
    class AtomicLinkedList;
   private:
    std::atomic<LinkedListNode<T> *> prev_;
    std::atomic<LinkedListNode<T> *> next_;
   public:

  };

  template<typename T>
  class AtomicLinkedList{
   private:
    AtomicLinkedListNode<T> *head_;
    AtomicLinkedListNode<T> *tail_;
   public:
    AtomicLinkedList():
        head_(new AtomicLinkedListNode<T>()),
        tail_(new AtomicLinkedListNode<T>()){
      head_->next_.store(tail_->next_);
      head_->prev_.store(nullptr);
      tail_->prev_.store(head_->next_);
      tail_->next_.store(nullptr);
    }
    ~AtomicLinkedList(){

    }

    void PushFront(const T &val){
      LinkedListNode<T> *node = new LinkedListNode<T>(val);
      node->prev_ = head_;

      AtomicLinkedListNode<T> *next;
      do{
        next = head_->next_.load();
        node->next_ = next;
      } while(!head_->next_.compare_exchange_strong(next, node));
      node->next_->prev_ = node;
    }

    void PushBack(const T &val){
      LinkedListNode<T> *node = new LinkedListNode<T>(val);
      node->next_ = tail_->next_;

      LinkedListNode<T> *prev;
      do{
        prev = tail_->prev_.load();
        node->prev_ = prev;
      } while(!tail_->prev_.compare_exchange_weak(prev, node));
      node->prev_->next_ = node;
    }

    bool IsEmpty() const{
      return head_->next_ == tail_;
    }

    T &PopFront(){
      LinkedListNode<T> *node;
      T val;
      do{
        node = head_->next_.load();
        val = node->value_;
      } while(!head_->next_.compare_exchange_weak(node, node->next_));
      node->next_->prev_ = head_;
      delete node;
      return std::move(val);
    }

    T &PopBack(){
      LinkedListNode<T> *node;
      T val;

      do{
        node = tail_->prev_.load();
        val = node->value_;
      } while(!tail_->prev_.compare_exchange_weak(node, node->prev_));
      node->prev_->next_ = tail_;
      delete node;
      return std::move(val);
    }
  };
}

#endif //TOKEN_ATOMIC_LINKED_LIST_H
