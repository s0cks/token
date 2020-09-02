#include "heap_dump.h"
#include "bitfield.h"
#include "bytes.h"

namespace Token{
    HeapDump::HeapDump(const std::string& filename, size_t semi_size):
        filename_(filename),
        eden_(new Heap(Space::kEdenSpace, semi_size)),
        survivor_(new Heap(Space::kSurvivorSpace, semi_size)){}

    HeapDump::HeapDump(const std::string& filename):
        filename_(filename),
        eden_(Allocator::GetEdenHeap()),
        survivor_(Allocator::GetSurvivorHeap()){}

    HeapDump::~HeapDump(){
        delete eden_;
        delete survivor_;
    }

    bool HeapDump::Accept(HeapDumpVisitor* vis){
        if(!vis->Visit(GetEdenHeap())) return false;
        if(!vis->Visit(GetSurvivorHeap())) return false;
        return true;
    }

    bool HeapDump::WriteHeapDump(const std::string& filename){
        HeapDumpWriter writer(filename);
        return writer.WriteHeapDump();
    }

    HeapDump* HeapDump::ReadHeapDump(const std::string& filename){
        HeapDumpReader reader(filename);
        return reader.ReadHeapDump();
    }

    bool HeapDumpWriter::Visit(RawObject* obj){
        WriteUnsignedLong(CreateObjectHeader(0, obj->GetAllocatedSize()));
        //TODO: WriteObject(obj);
        return true;
    }

    bool HeapDumpWriter::Visit(RawObject** root){
        RawObject* obj = (*root);
        if(!obj) return false;
        WriteUnsignedLong(CreateObjectHeader(0, obj->GetAllocatedSize()));
        //TODO:WriteObject(obj);
        return true;
    }

    bool HeapDumpWriter::WriteStackSpace(){
        LOG(INFO) << "writing stack space to heap dump....";
#ifdef TOKEN_DEBUG
        LOG(INFO) << "Size: " << Allocator::GetStackSpaceSize();
        LOG(INFO) << "Number of Objects: " << Allocator::GetNumberOfStackSpaceObjects();
#endif//TOKEN_DEBUG

        // Write the Header
        WriteUnsignedLong(CreateSectionHeader(Space::kStackSpace, Allocator::GetStackSpaceSize()));
        // Write the Data
        WriteUnsignedLong(Allocator::GetNumberOfStackSpaceObjects());
        return Allocator::VisitRoots(this);
    }

    class HeapDumpHeapDataWriter : public ObjectPointerVisitor{
    private:
        HeapDumpWriter* writer_;
        Heap* heap_;
        ByteBuffer bytes_;
    public:
        HeapDumpHeapDataWriter(HeapDumpWriter* parent, Heap* heap):
            ObjectPointerVisitor(),
            writer_(parent),
            bytes_(heap->GetTotalSize()){}
        ~HeapDumpHeapDataWriter() = default;

        HeapDumpWriter* GetWriter() const{
            return writer_;
        }

        Heap* GetHeap() const{
            return heap_;
        }

        bool Visit(RawObject* obj){
            //TODO: GetWriter()->WriteObject(obj);
            return true;
        }

        bool Write(){
            return GetHeap()->Accept(this);
        }
    };

    bool HeapDumpWriter::WriteHeap(Heap* heap){
        LOG(INFO) << "writing " << heap->GetSpace() << heap->GetAllocatedSize() << "/" << heap->GetTotalSize() << " space to heap dump....";

        // Write the Header
        LOG(INFO) << "writing heap section header....";
        WriteUnsignedLong(CreateSectionHeader(heap->GetSpace(), heap->GetAllocatedSize()));

        // Write the Data
        LOG(INFO) << "writing heap data section....";
        HeapDumpHeapDataWriter writer(this, heap);
        if(!writer.Write()){
            LOG(WARNING) << "couldn't write heap data to heap dump";
            return false;
        }
        return true;
    }

    bool HeapDumpWriter::WriteHeapDump(){
        WriteUnsignedLong(GetCurrentTimestamp()); // Generation Timestamp
        WriteUnsignedLong(Allocator::kSemispaceSize); // Semispace Size

        // Write Stack Space Section
        LOG(INFO) << "writing stack space....";
        if(!WriteStackSpace()){
            LOG(WARNING) << "couldn't write heap dump stack space section";
            return false;
        }

        // Write Eden Heap Section
        LOG(INFO) << "writing eden heap....";
        if(!WriteHeap(Allocator::GetEdenHeap())){
            LOG(WARNING) << "couldn't write heap dump eden space section";
            return false;
        }

        // Write Survivor Heap Section
        LOG(INFO) << "writing survivor heap....";
        if(!WriteHeap(Allocator::GetSurvivorHeap())){
            LOG(WARNING) << "couldn't write heap dump survivor space section";
            return false;
        }
        return true;
    }

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
                case Space::kEdenSpace:{
                    LOG(INFO) << "reading eden space....";
                    Heap* heap = dump->GetEdenHeap();
                    if(!ReadMemoryRegion(heap->GetRegion(), size)){
                        LOG(WARNING) << "couldn't read space memory region from file";
                        delete dump;
                        return nullptr;
                    }
                    break;
                }
                case Space::kSurvivorSpace:{
                    LOG(INFO) << "reading survivor space....";
                    //TODO: implement
                    break;
                }
                default:
                    LOG(WARNING) << "unknown space: " << space;
                    delete dump;
                    return nullptr;
            }
        }
    }
}