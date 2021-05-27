#ifndef TOKEN_ATOMIC_LINKED_LIST_H
#define TOKEN_ATOMIC_LINKED_LIST_H

#include <memory>

namespace token{
  template<class T>
  class AtomicLinkedList{
  public:
    struct Node;
    typedef std::shared_ptr<Node> NodePtr;

    struct Node{
      T value;
      std::shared_ptr<Node> next;

      Node(const T& val, const NodePtr& previous):
        value(val),
        next(previous){}
      explicit Node(const T& val):
        Node(val, NodePtr()){}
    };

    typedef std::atomic<NodePtr> AtomicNodePtr;
  private:
    AtomicNodePtr head_;
  public:
    AtomicLinkedList() = default;
    ~AtomicLinkedList() = default;

    AtomicNodePtr head() const{
      return head_;
    }

    AtomicNodePtr& head(){
      return head_;
    }

    void push_front(const T& val){
      auto p = std::make_shared<Node>(val, head_);
      while(head_.compare_exchange_weak(p->next, p)){}
    }

    void pop_front(){
      auto p = head_.load();
      while(p && !head_.compare_exchange_weak(p, p->next)){}
    }
  };
}

#endif//TOKEN_ATOMIC_LINKED_LIST_H