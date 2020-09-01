#ifndef TOKEN_NODE_H
#define TOKEN_NODE_H

#include "object.h"

namespace Token{
    class Node : public Object{
    private:
        Node* previous_;
        Node* next_;
    protected:
        Node():
            Object(),
            previous_(nullptr),
            next_(nullptr){}
        Node(Node* previous, Node* next):
            Object(),
            previous_(previous),
            next_(next){}

        void Accept(WeakReferenceVisitor* vis){
            vis->Visit(&previous_);
            vis->Visit(&next_);
        }
    public:
        ~Node(){}

        void SetPrevious(const Handle<Node>& node){
            previous_ = node;
        }

        Handle<Node> GetPrevious() const{
            return previous_;
        }

        bool HasPrevious() const{
            return previous_ != nullptr;
        }

        void SetNext(const Handle<Node>& node){
            next_ = node;
        }

        Handle<Node> GetNext() const{
            return next_;
        }

        bool HasNext() const{
            return next_ != nullptr;
        }
    };
}

#endif //TOKEN_NODE_H