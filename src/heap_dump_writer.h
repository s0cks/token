#ifndef TOKEN_HEAP_DUMP_WRITER_H
#define TOKEN_HEAP_DUMP_WRITER_H

#include "heap_dump.h"
#include "file_writer.h"

namespace Token{
    class HeapDumpWriter : public BinaryFileWriter{
    private:
        static inline std::string
        GetNewHeapDumpFilename(){
            std::stringstream filename;
            filename << HeapDump::GetHeapDumpDirectory();
            filename << "/dump-" << GetTimestampFormattedFileSafe() << ".dat";
            return filename.str();
        }
    public:
        HeapDumpWriter(const std::string& filename=GetNewHeapDumpFilename()):
                BinaryFileWriter(filename){}
        ~HeapDumpWriter() = default;

        bool WriteHeapDump();
    };
}

#endif //TOKEN_HEAP_DUMP_WRITER_H