#ifndef TOKEN_CRASH_REPORT_WRITER_H
#define TOKEN_CRASH_REPORT_WRITER_H

#include "file_writer.h"
#include "crash_report.h"

namespace Token{
    class CrashReportWriter : public TextFileWriter{
    private:
        template<class Pool>
        inline bool
        WritePool(const std::string& name){
            std::stringstream line;
            line << name << " Pool: ";
            size_t cache_size = Pool::GetCacheSize();
            size_t cache_max_size = Pool::GetMaxCacheSize();
            double cache_util = (cache_size/cache_max_size);
            line << "Cache Utilization: " << std::fixed << std::setprecision(2) << cache_util << "% ";
            line << "(" << cache_size << "/" << cache_max_size << " Objects)";
            return WriteLine(line);
        }

        bool WriteBanner();
        bool WriteSystemInformation();
        bool WriteServerInformation();
    public:
        CrashReportWriter(const std::string& filename):
            TextFileWriter(filename){}
        ~CrashReportWriter() = default;

        bool WriteCrashReport();
    };
}

#endif //TOKEN_CRASH_REPORT_WRITER_H