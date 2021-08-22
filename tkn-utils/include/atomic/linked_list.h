#ifndef TKN_ATOMIC_LINKED_LIST_H
#define TKN_ATOMIC_LINKED_LIST_H

#include <atomic>
#include <memory>

namespace token{
  namespace atomic{
    template<class E>
    class LinkedList{
    public:
      struct Node{
        std::shared_ptr<Node> next;
        std::shared_ptr<E> data;

        Node(const std::shared_ptr<Node>& parent, std::shared_ptr<E> val):
          next(parent),
          data(std::move(val)){
        }
      };
    private:
      std::shared_ptr<Node> head_;
      std::atomic<size_t> size_;

      std::shared_ptr<Node> head(){
        return std::atomic_load(&head_);
      }
    public:
      LinkedList():
        head_(nullptr),
        size_(0){}
      ~LinkedList() = default;

      size_t size() const{
        return size_;
      }

      bool empty(){
        return head().get() == nullptr;
      }

      void push_front(const std::shared_ptr<E>& val){
        auto n = std::make_shared<Node>(head(), val);
        std::atomic_store(&head_, n);
        size_ += 1;
      }

      std::shared_ptr<E> pop_front(){
        auto n = head();
        std::atomic_store(&head_, n->next);
        size_ -= 1;
        return n->data;
      }
    };
  }
}

#endif//TKN_ATOMIC_LINKED_LIST_H