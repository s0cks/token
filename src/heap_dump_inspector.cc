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

    class HeapObjectPrinter : public ObjectPointerVisitor{
    public:
        HeapObjectPrinter() = default;
        ~HeapObjectPrinter() = default;

        bool Visit(RawObject* obj){
            LOG(INFO) << " - " << obj->ToString();
            return true;
        }

        static inline void
        Print(Heap* heap){
            HeapObjectPrinter printer;
            if(!heap->Accept(&printer)){
                LOG(WARNING) << "couldn't iterate over heap objects";
            }
        }
    };

    void HeapDumpInspector::HandleTestCommand(HeapDumpInspectorCommand* cmd){
        Heap* heap = GetHeapByName(cmd->GetNextArgument());
        PrintHeap(heap);
        HeapObjectPrinter::Print(heap);
    }
}