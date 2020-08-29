#ifndef TOKEN_HEAP_DUMP_H
#define TOKEN_HEAP_DUMP_H

#include "heap.h"

namespace Token{
    class HeapDumpWriter;
    class HeapDumpReader;
    class HeapDump{
    private:
        std::string filename_;

        static std::string GenerateNewFilename();
    public:
        ~HeapDump() = default;

        std::string GetFilename() const{
            return filename_;
        }

        void Accept(HeapDumpWriter* writer);
        void Accept(HeapDumpReader* reader);

        static bool WriteHeapDump(const std::string& filename=GenerateNewFilename());
        static HeapDump* ReadHeapDump(const std::string& filename);
    };
}

#endif //TOKEN_HEAP_DUMP_H