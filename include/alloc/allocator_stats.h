#ifndef TOKEN_STATS_H
#define TOKEN_STATS_H

#include <cstdint>

namespace Token{
    class AllocatorObjectStats{
        friend class Object;
    private:
        int64_t sequence_;
        int64_t num_references_;
        int64_t num_collections_;

        AllocatorObjectStats():
            sequence_(0),
            num_references_(0),
            num_collections_(0){}
    public:
        AllocatorObjectStats(const AllocatorObjectStats& stats):
            sequence_(stats.sequence_),
            num_references_(stats.num_references_),
            num_collections_(stats.num_collections_){}
        ~AllocatorObjectStats() = default;

        int64_t GetSequence() const{
            return sequence_;
        }

        int64_t GetNumberOfReferences() const{
            return num_references_;
        }

        int64_t GetNumberOfCollectionsSurvived() const{
            return num_collections_;
        }

        bool HasReferences() const{
            return num_references_ > 0;
        }

        void operator=(const AllocatorObjectStats& stats){
            sequence_ = stats.sequence_;
            num_references_ = stats.num_references_;
            num_collections_ = stats.num_collections_;
        }
    };

    class AllocatorStats{};
}

#endif //TOKEN_STATS_H