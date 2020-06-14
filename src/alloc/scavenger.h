#ifndef TOKEN_SCAVENGER_H
#define TOKEN_SCAVENGER_H

#include "object.h"

namespace Token{
    class GCMarker{
    private:
        Color color_;
    public:
        GCMarker(Color color):
            color_(color){}
        ~GCMarker(){}

        Color GetColor() const{
            return color_;
        }

        void MarkObject(RawObject* obj){
            obj->SetColor(GetColor());
        }

        GCMarker& operator=(const GCMarker& other){
            color_ = other.color_;
            return (*this);
        }
    };

    class Heap;
    class Semispace;
    class Scavenger{
    private:
        Heap* heap_;

        RawObject* Evacuate(RawObject* obj);

        friend class HeapObjectPointerVisitor;
    public:
        Scavenger(Heap* heap):
            heap_(heap){}
        ~Scavenger(){}

        Heap* GetHeap() const{
            return heap_;
        }

        Semispace* GetFromSpace() const;
        Semispace* GetToSpace() const;

        bool DarkenRoots();
        bool EvacuateObjects();

        static bool Scavenge(Heap* heap);
    };
}

#endif //TOKEN_SCAVENGER_H