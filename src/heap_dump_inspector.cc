#include "heap_dump_inspector.h"

namespace Token{
    static inline void
    PrintHeap(Heap* heap){
        LOG(INFO) << heap->GetSpace() << " Heap:";
        LOG(INFO) << "  Allocated Size (Bytes): " << heap->GetAllocatedSize();
        LOG(INFO) << "  Unallocated Size (Bytes): " << heap->GetUnallocatedSize();
        LOG(INFO) << "  Total Size (Bytes): " << heap->GetTotalSize();
    }

    void HeapDumpInspector::HandleStatusCommand(HeapDumpInspectorCommand* cmd){
        LOG(INFO) << "Heap Dump:";
        LOG(INFO) << "Filename: " << GetData()->GetFilename();
        PrintHeap(GetData()->GetEdenHeap());
        PrintHeap(GetData()->GetSurvivorHeap());
    }

    void HeapDumpInspector::HandleGetHeapCommand(HeapDumpInspectorCommand* cmd){

    }
}