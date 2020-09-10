#include "heap_dump.h"
#include "bytes.h"
#include "bitfield.h"
#include "heap_dump_reader.h"
#include "heap_dump_writer.h"

namespace Token{
    HeapDump::HeapDump(const std::string& filename, size_t semi_size):
        filename_(filename),
        eden_(nullptr),
        survivor_(nullptr){}

    HeapDump::HeapDump(const std::string& filename):
        filename_(filename),
        eden_(Allocator::GetNewSpace()),
        survivor_(Allocator::GetOldSpace()){}

    HeapDump::~HeapDump(){
        delete eden_;
        delete survivor_;
    }

    bool HeapDump::Accept(HeapDumpVisitor* vis){
        if(!vis->Visit(GetEdenHeap())){
            LOG(WARNING) << "couldn't visit eden heap";
            return false;
        }

        if(!vis->Visit(GetSurvivorHeap())){
            LOG(WARNING) << "couldn't visit survivor heap";
            return false;
        }
        return true;
    }

    bool HeapDump::WriteNewHeapDump(const std::string& filename){
        HeapDumpWriter writer(filename);
        return writer.WriteHeapDump();
    }

    HeapDump* HeapDump::ReadHeapDump(const std::string& filename){
        HeapDumpReader reader(filename);
        return reader.ReadHeapDump();
    }
}