#ifndef TOKEN_SCAVENGER_H
#define TOKEN_SCAVENGER_H

#include "raw_object.h"

namespace Token{
    class Marker : public ObjectPointerVisitor{
    private:
        Color color_;

        void MarkObject(RawObject* obj){
            obj->SetColor(GetColor());
        }
    public:
        Marker(Color color):
            color_(color){}
        ~Marker(){}

        Color GetColor() const{
            return color_;
        }

        bool Visit(RawObject* ptr){
            if(GetColor() != ptr->GetColor()) MarkObject(ptr);
            return true;
        }

        Marker& operator=(const Marker& other){
            color_ = other.color_;
            return (*this);
        }
    };

    class Heap;
    class Semispace;
    class Scavenger{
    protected:
        Heap* heap_;

        bool DarkenRoots();
        bool MarkObjects();
        virtual bool ScavengeMemory() = 0;

        Scavenger(Heap* heap):
            heap_(heap){}
    public:
        virtual ~Scavenger() = default;

        Heap* GetHeap() const{
            return heap_;
        }

        static bool Scavenge(Heap* heap);
    };

    class MarkCopyScavenger : public Scavenger{
    protected:
        RawObject* Evacuate(RawObject* obj);

        friend class MarkCopyMemoryScavenger;
    public:
        MarkCopyScavenger(Heap* heap): Scavenger(heap){}
        ~MarkCopyScavenger(){}

        Semispace* GetFromSpace() const;
        Semispace* GetToSpace() const;
        bool ScavengeMemory();
    };
}

#endif //TOKEN_SCAVENGER_H