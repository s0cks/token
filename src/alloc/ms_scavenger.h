#ifndef TOKEN_MS_SCAVENGER_H
#define TOKEN_MS_SCAVENGER_H

#include "crash_report.h"
#include "scavenger.h"
#include "allocator.h"
#include "object.h"

namespace Token{
    class MarkSweepScavenger : public Scavenger{
    public:
        MarkSweepScavenger(): Scavenger(){}
        ~MarkSweepScavenger(){}

        bool DeleteObject(RawObject* obj){
            if(obj->GetColor() != Color::kWhite) return true;
            if(!((Object*)obj->GetObjectPointer())->Finalize()){
                std::stringstream ss;
                ss << "Couldn't finalize object: " << obj;
                CrashReport::GenerateAndExit(ss.str());
            }
            return Allocator::DeleteObject(obj);
        }

        bool ScavengeMemory();
    };
}

#endif //TOKEN_MS_SCAVENGER_H