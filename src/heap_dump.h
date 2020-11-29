#ifndef TOKEN_GCMODE_NONE
#ifndef TOKEN_HEAP_DUMP_H
#define TOKEN_HEAP_DUMP_H

#include "heap.h"
#include "command.h"
#include "inspector.h"

namespace Token{
    class HeapDump{
        friend class HeapDumpReader;
        friend class HeapDumpWriter;
    private:
        std::string filename_;
        std::string version_;
        Timestamp timestamp_;
        MemoryRegion* region_;
        Heap* new_heap_;
        Heap* old_heap_;

        HeapDump(const std::string& filename, const std::string& version, Timestamp timestamp, MemoryRegion* region, intptr_t heap_size, intptr_t semispace_size):
            filename_(filename),
            version_(version),
            timestamp_(timestamp),
            region_(region),
            new_heap_(new Heap(Space::kNewHeap, region->GetStartAddress(), heap_size, semispace_size)),
            old_heap_(new Heap(Space::kOldHeap, region->GetStartAddress()+heap_size, heap_size, semispace_size)){}
    public:
        ~HeapDump(){}

        std::string GetFilename() const{
            return filename_;
        }

        std::string GetVersion() const{
            return version_;
        }

        Timestamp GetTimestamp() const{
            return timestamp_;
        }

        MemoryRegion* GetRegion() const{
            return region_;
        }

        Heap* GetNewHeap() const{
            return new_heap_;
        }

        Heap* GetOldHeap() const{
            return old_heap_;
        }

        static bool WriteHeapDump();
        static HeapDump* ReadHeapDump(const std::string& filename);

        static inline std::string
        GetHeapDumpDirectory(){
            return TOKEN_BLOCKCHAIN_HOME;
        }
    };

#define FOR_EACH_HEAP_INSPECTOR_COMMAND(V) \
    V(Status, ".status", 0) \
    V(PrintObjects, "print-objects", 1)

    class HeapDumpInspectorCommand : public Command{
    public:
        HeapDumpInspectorCommand(std::deque<std::string>& args):
            Command(args){}
        ~HeapDumpInspectorCommand() = default;

        Space GetNextArgumentSpace(){
            std::string next = GetNextArgument();
            std::transform(next.begin(), next.end(), next.begin(), [](unsigned char c){
                return std::tolower(c);
            });
            if(next == "stack"){
                return Space::kStackSpace;
            } else if(next == "new"){
                return Space::kNewHeap;
            } else if(next == "old"){
                return Space::kNewHeap;
            }

            LOG(WARNING) << "unknown space: " << next;
            return Space::kNewHeap;
        }

#define DEFINE_TYPE_CHECK(Name, Text, ArgumentCount) \
        bool Is##Name##Command() const{ return !strncmp(name_.data(), Text, strlen(Text)); }
        FOR_EACH_HEAP_INSPECTOR_COMMAND(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
    };

    class HeapDumpInspector : public Inspector<HeapDump, HeapDumpInspectorCommand>{
    private:
        inline HeapDump*
        GetHeapDump() const{
            return (HeapDump*)data_;
        }

        inline bool
        HasHeapDump() const{
            return data_ != nullptr;
        }

        inline Heap* GetOldHeap() const{
            return GetHeapDump()->GetOldHeap();
        }

        inline Heap* GetNewHeap() const{
            return GetHeapDump()->GetNewHeap();
        }

#define DECLARE_HANDLER(Name, Text, Parameters) \
        void Handle##Name##Command(HeapDumpInspectorCommand* cmd);
        FOR_EACH_HEAP_INSPECTOR_COMMAND(DECLARE_HANDLER)
#undef DECLARE_HANDLER

        void OnCommand(HeapDumpInspectorCommand* cmd){
#define DEFINE_TYPE_CHECK(Name, Text, ArgumentCount) \
            if(cmd->Is##Name##Command()){ \
                Handle##Name##Command(cmd); \
                return; \
            }
            FOR_EACH_HEAP_INSPECTOR_COMMAND(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
        }
    public:
        HeapDumpInspector():
            Inspector(){}
        ~HeapDumpInspector() = default;
    };
}

#endif //TOKEN_HEAP_DUMP_H
#endif //!TOKEN_GCMODE_NONE