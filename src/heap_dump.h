#ifndef TOKEN_HEAP_DUMP_H
#define TOKEN_HEAP_DUMP_H

#include "heap.h"
#include "bitfield.h"
#include "file_writer.h"
#include "file_reader.h"

namespace Token{
    class HeapDumpWriter;
    class HeapDumpReader;
    class HeapDumpVisitor;
    class HeapDump{
        friend class HeapDumpWriter;
        friend class HeapDumpReader;
    private:
        std::string filename_;
        Heap* eden_;
        Heap* survivor_;

        HeapDump(const std::string& filename, size_t semi_size);
        HeapDump(const std::string& filename);

        static inline std::string
        GenerateNewFilename(){
            std::stringstream filename;
            filename << GetHeapDumpDirectory();
            filename << "/heap-dump-" << GetTimestampFormattedFileSafe(GetCurrentTimestamp()) << ".dat";
            return filename.str();
        }
    public:
        ~HeapDump();

        std::string GetFilename() const{
            return filename_;
        }

        Heap* GetEdenHeap() const{
            return eden_;
        }

        Heap* GetSurvivorHeap() const{
            return survivor_;
        }

        bool Accept(HeapDumpVisitor* vis);

        static bool WriteNewHeapDump(const std::string& filename=GenerateNewFilename());
        static HeapDump* ReadHeapDump(const std::string& filename);

        static inline std::string
        GetHeapDumpDirectory(){
            return (TOKEN_BLOCKCHAIN_HOME);
        }
    };

    class HeapDumpVisitor{
    protected:
        HeapDumpVisitor() = default;
    public:
        virtual ~HeapDumpVisitor() = default;
        virtual bool Visit(Heap* heap) = 0;
    };

    typedef uint64_t HeapDumpSectionHeader;
    enum HeapDumpSectionHeaderLayout{
        kSectionSpaceFieldPosition = 0,
        kSectionSpaceFieldSize = 16,
        kSectionSizeFieldPosition = kSectionSpaceFieldPosition+kSectionSpaceFieldSize,
        kSectionSizeFieldSize = 32,
    };

    class SectionSpaceField : public BitField<HeapDumpSectionHeader, Space, kSectionSpaceFieldPosition, kSectionSpaceFieldSize>{};
    class SectionSizeField : public BitField<HeapDumpSectionHeader, uint32_t, kSectionSizeFieldPosition, kSectionSizeFieldSize>{};

    static inline HeapDumpSectionHeader
    CreateSectionHeader(Space space, uint32_t size){
        HeapDumpSectionHeader header = 0;
        header = SectionSpaceField::Update(space, header);
        header = SectionSizeField::Update(size, header);
        return header;
    }

    static inline std::string
    GetSectionHeaderInformation(HeapDumpSectionHeader header){
        std::stringstream stream;
        switch(SectionSpaceField::Decode(header)){
            case Space::kStackSpace:
                stream << "Stack Space (" << SectionSizeField::Decode(header) << " Bytes)";
                break;
            case Space::kEdenSpace:
                stream << "Eden Space (" << SectionSizeField::Decode(header) << " Bytes)";
                break;
            default:
                stream << "Unknown Space (" << SectionSizeField::Decode(header) << " Bytes)";
                break;
        }
        return stream.str();
    }

    typedef uint64_t HeapDumpObjectHeader;
    enum HeapDumpObjectHeaderLayout{
        kObjectTypeFieldPosition = 0,
        kObjectTypeFieldSize = 16,
        kObjectSizeFieldPosition = kObjectTypeFieldPosition+kObjectTypeFieldSize,
        kObjectSizeFieldSize = 32,
    };

    class ObjectTypeField : public BitField<HeapDumpObjectHeader, uint16_t, kObjectTypeFieldPosition, kObjectTypeFieldSize>{};
    class ObjectSizeField : public BitField<HeapDumpObjectHeader, uint32_t, kObjectSizeFieldPosition, kObjectSizeFieldSize>{};

    static inline HeapDumpObjectHeader
    CreateObjectHeader(uint16_t type, uint32_t size){
        HeapDumpObjectHeader header = 0;
        header = ObjectTypeField::Update(type, header);
        header = ObjectSizeField::Update(size, header);
        return header;
    }
}

#endif //TOKEN_HEAP_DUMP_H