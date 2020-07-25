#include "scavenger.h"

namespace Token{
    template<typename I>
    void Scavenger::MarkObjects(Iterable<I> iterable){
        for(Object* obj : iterable){ //TODO: fixme, poor design
            if(obj->HasStackReferences()){
                obj->SetColor(Object::kBlack);
            }
        }
    }

    template<typename I>
    void Scavenger::FinalizeObjects(Iterable<I> iterable){
        for(Object* obj : iterable){ //TODO: fixme, poor design
            if(obj->IsGarbage()){
                obj->~Object(); //TODO: call finalizer
                obj->ptr_ = nullptr;
            }
        }
    }

    //TODO:
    // - smarter relocation based on zoning
    template<typename I>
    void Scavenger::RelocateObjects(Iterable<I> iterable){
        for(Object* obj : iterable){
            if(!obj->IsGarbage()){
                size_t size = obj->GetSize();

                void* nptr = nullptr;
                if(obj->GetStats()->GetNumberOfCollectionsSurvived() >= Scavenger::kNumberOfCollectionsForPromotion){
                    nptr = Allocator::GetSurvivorHeap()->GetToSpace()->Allocate(size);
                } else{
                    nptr = Allocator::GetEdenHeap()->GetToSpace()->Allocate(size);
                    obj->GetStats()->survived_++;
                }
                obj->ptr_ = static_cast<Object*>(nptr);
            }
        }
    }

    class WeakReferenceNotifier : public FieldIterator{
    private:
        Object* target_;
    public:
        WeakReferenceNotifier(Object* target): target_(target){}

        void operator()(Object** field) const{
            Object* obj = (*field);
            if(!obj) return;
            if(!obj->ptr_){
                (*field) = nullptr;
                target_->NotifyWeakReferences(field);
            }
        }
    };

    template<bool AsRoot, typename I>
    void Scavenger::NotifyWeakReferences(Iterable<I> iterable){
        for(Object* obj : iterable){
            if(AsRoot || !obj->IsGarbage()) obj->Accept(WeakReferenceNotifier{ obj });
        }
    }

    class UpdateIterator : public FieldIterator{
    public:
        void operator()(Object** field) const{
            Object* obj = (*field);
            if(!obj) return;
            (*field) = obj->ptr_;
        }
    };

    template<typename I>
    void Scavenger::UpdateNonRootReferences(Iterable<I> iterable){
        for(Object* obj : iterable){
            if(!obj->IsGarbage()) obj->Accept(UpdateIterator{});
        }
    }

    template<typename I>
    void Scavenger::CopyLiveObjects(Iterable<I> iterable){
        for(Object* obj : iterable){
            if(!obj->IsGarbage()){
                obj->SetColor(Object::kFree);
                memcpy((void*)obj->ptr_, (void*)obj, obj->GetSize());
            }
        }
    }

    void Scavenger::Scavenge(){
        MarkObjects<SemispaceIterator>(Allocator::GetEdenHeap()->GetFromSpace());
        MarkObjects<SemispaceIterator>(Allocator::GetSurvivorHeap()->GetFromSpace());

        FinalizeObjects<SemispaceIterator>(Allocator::GetEdenHeap()->GetFromSpace());
        FinalizeObjects<SemispaceIterator>(Allocator::GetSurvivorHeap()->GetFromSpace());

        RelocateObjects<SemispaceIterator>(Allocator::GetEdenHeap()->GetFromSpace());
        RelocateObjects<SemispaceIterator>(Allocator::GetSurvivorHeap()->GetFromSpace());

        NotifyWeakReferences<false, SemispaceIterator>(Allocator::GetEdenHeap()->GetFromSpace());
        NotifyWeakReferences<false, SemispaceIterator>(Allocator::GetSurvivorHeap()->GetFromSpace());
        Allocator::NotifyWeakStackReferences();

        Allocator::UpdateStackReferences();
        UpdateNonRootReferences<SemispaceIterator>(Allocator::GetEdenHeap()->GetFromSpace());
        UpdateNonRootReferences<SemispaceIterator>(Allocator::GetSurvivorHeap()->GetFromSpace());

        CopyLiveObjects<SemispaceIterator>(Allocator::GetEdenHeap()->GetFromSpace());
        CopyLiveObjects<SemispaceIterator>(Allocator::GetSurvivorHeap()->GetFromSpace());

        Allocator::GetEdenHeap()->GetFromSpace()->Reset();
        Allocator::GetEdenHeap()->SwapSpaces();

        Allocator::GetSurvivorHeap()->GetFromSpace()->Reset();
        Allocator::GetSurvivorHeap()->SwapSpaces();
    }
}