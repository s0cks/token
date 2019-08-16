#include "alloc.h"

namespace Token{
    Allocator::Allocator(){

    }

    Allocator::~Allocator(){

    }

    Allocator* Allocator::GetInstance(){
        static Allocator instance;
        return &instance;
    }
}