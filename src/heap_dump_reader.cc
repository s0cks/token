#include "heap_dump_reader.h"

namespace Token{
    bool HeapDumpReader::ReadMemoryRegion(MemoryRegion* region, size_t size){
        LOG(INFO) << "reading memory region of size: " << size << " from heap dump...";
        return ReadBytes((uint8_t*)region->GetStartAddress(), size);
    }

    HeapDump* HeapDumpReader::ReadHeapDump(){
        uint64_t timestamp = ReadUnsignedLong();
        uint64_t semi_size = ReadUnsignedLong();

        LOG(INFO) << "Timestamp: " << GetTimestampFormattedReadable(timestamp);
        LOG(INFO) << "Semispace Size (Bytes): " << semi_size;

        HeapDump* dump = new HeapDump(GetFilename(), semi_size);
        while(true){
            HeapDumpSectionHeader header = ReadSectionHeader();
            size_t size = SectionSizeField::Decode(header);
            Space space = SectionSpaceField::Decode(header);
            switch(space){
                case Space::kStackSpace:{
                    size_t num_objects = ReadLong();
                    LOG(INFO) << "reading " << num_objects << " stack space objects....";
                    break;
                }
                case Space::kNewSpace:{
                    LOG(INFO) << "reading new space....";
                    Heap* heap = dump->GetHeap();
                    /*if(!ReadMemoryRegion(heap->GetRegion(), size)){
                        LOG(WARNING) << "couldn't read space memory region from file";
                        delete dump;
                        return nullptr;
                    }*/
                    heap->SetAllocatedSize(size);
                    break;
                }
                case Space::kOldSpace: return dump;
                default:
                    LOG(WARNING) << "unknown space: " << space;
                    delete dump;
                    return nullptr;
            }
        }
    }
}