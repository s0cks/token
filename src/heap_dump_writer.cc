#include "heap_dump_writer.h"

namespace Token{
    bool HeapDumpWriter::WriteStackSpace(){
        LOG(INFO) << "writing stack space to heap dump....";
        // Write the Header
        WriteUnsignedLong(CreateSectionHeader(Space::kStackSpace, Allocator::GetStackSpaceSize()));
        // Write the Data
        WriteUnsignedLong(Allocator::GetNumberOfStackSpaceObjects());
        return true;
    }

    bool HeapDumpWriter::WriteHeap(Heap* heap){
        LOG(INFO) << "writing " << heap->GetSpace() << heap->GetTotalSize() << " space to heap dump....";
        // Write the Header
        LOG(INFO) << "writing heap section header....";
        WriteUnsignedLong(CreateSectionHeader(heap->GetSpace(), heap->GetAllocatedSize()));

        // Write the Data
        LOG(INFO) << "writing heap section data....";
        /*if(!WriteBytes(heap->GetRegion(), heap->GetAllocatedSize())){
            LOG(WARNING) << "couldn't write heap memory region to section data";
            return false;
        }*/
        return true;
    }

    bool HeapDumpWriter::WriteHeapDump(){
        LOG(INFO) << "writing heap dump....";
        WriteUnsignedLong(GetCurrentTimestamp()); // Generation Timestamp
        //WriteUnsignedLong(Allocator::kSemispaceSize); // Semispace Size

        // Write Stack Space Section
        LOG(INFO) << "writing stack space....";
        if(!WriteStackSpace()){
            LOG(WARNING) << "couldn't write heap dump stack space section";
            return false;
        }

        // Write Eden Heap Section
        LOG(INFO) << "writing heap....";
        if(!WriteHeap(Allocator::GetHeap())){
            LOG(WARNING) << "couldn't write heap dump eden space section";
            return false;
        }

        LOG(INFO) << "heap dump written to: " << GetFilename();
        return true;
    }
}