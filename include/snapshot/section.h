#ifndef TOKEN_INCLUDE_SNAPSHOT_SECTION_H
#define TOKEN_INCLUDE_SNAPSHOT_SECTION_H

#include "version.h"
#include "blockchain.h"

namespace Token{
  typedef uint64_t SnapshotSectionHeader;

  class SnapshotWriter;
  class SnapshotReader;
  class SnapshotSection{
   public:
    enum Type{
      kPrologue,
      kBlockChain,
    };

    friend std::ostream& operator<<(std::ostream& stream, const Type& type){
      switch(type){
        case Type::kPrologue:
          return stream << "Prologue";
        case Type::kBlockChain:
          return stream << "BlockChain";
        default:
          return stream << "Unknown";
      }
    }
   private:
    enum HeaderLayout{
      kTypePos = 0,
      kBitsForType = 32,

      kSizePos = kTypePos+kBitsForType,
      kBitsForSize = 32,
    };

    class TypeField : public BitField<uint64_t, SnapshotSection::Type, kTypePos, kBitsForType>{};
    class SizeField : public BitField<uint64_t, int32_t, kSizePos, kBitsForSize>{};
   protected:
    Type type_;

    SnapshotSection(const Type& type):
      type_(type){}
   public:
    virtual ~SnapshotSection() = default;

    Type GetType() const{
      return type_;
    }

    SnapshotSectionHeader GetSectionHeader() const{
      return TypeField::Encode(type_)|SizeField::Encode(GetSize());
    }

    virtual int32_t GetSize() const = 0;
    virtual bool Write(SnapshotWriter* writer) const = 0;

    static bool IsPrologueSection(const SnapshotSectionHeader& header){
      LOG(INFO) << "type: " << TypeField::Decode(header);
      return TypeField::Decode(header) == Type::kPrologue;
    }

    static bool IsBlockChainSection(const SnapshotSectionHeader& header){
      return TypeField::Decode(header) == Type::kBlockChain;
    }

    static int32_t GetSectionSize(const SnapshotSectionHeader& header){
      return SizeField::Decode(header);
    }
  };

  class SnapshotPrologueSection : public SnapshotSection{
   private:
    Timestamp timestamp_;
    Version version_;
   public:
    SnapshotPrologueSection(const Timestamp& timestamp, const Version& version):
      SnapshotSection(SnapshotSection::kPrologue),
      timestamp_(timestamp),
      version_(version){}
    SnapshotPrologueSection():
      SnapshotSection(SnapshotSection::kPrologue),
      timestamp_(GetCurrentTimestamp()),
      version_(){}
    SnapshotPrologueSection(SnapshotReader* reader);
    SnapshotPrologueSection(const SnapshotPrologueSection& section):
      SnapshotSection(SnapshotSection::kPrologue),
      timestamp_(section.timestamp_),
      version_(section.version_){}
    ~SnapshotPrologueSection() = default;

    Timestamp GetTimestamp() const{
      return timestamp_;
    }

    Version GetVersion() const{
      return version_;
    }

    int32_t GetSize() const{
      int32_t size = 0;
      size += sizeof(Timestamp);
      size += Version::kSize;
      return size;
    }

    bool Write(SnapshotWriter* writer) const;

    void operator=(const SnapshotPrologueSection& section){
      timestamp_ = section.timestamp_;
      version_ = section.version_;
    }
  };

  class SnapshotBlockChainSection : public SnapshotSection{
   public:
    class Reference{
     public:
      struct PositionComparator{
        bool operator()(const Reference& a, const Reference& b){
          return a.GetPosition() < b.GetPosition();
        }
      };
     private:
      Hash hash_;
      int64_t pos_;
      int64_t size_;
     public:
      Reference(const Hash& hash, int64_t pos, int64_t size):
        hash_(hash),
        pos_(pos),
        size_(size){}
      Reference(const Reference& ref):
        hash_(ref.hash_),
        pos_(ref.pos_),
        size_(ref.size_){}
      ~Reference() = default;

      Hash GetHash() const{
        return hash_;
      }

      int64_t GetPosition() const{
        return pos_;
      }

      int64_t GetSize() const{
        return size_;
      }

      void operator=(const Reference& ref){
        hash_ = ref.hash_;
        pos_ = ref.pos_;
        size_ = ref.size_;
      }

      friend bool operator==(const Reference& a, const Reference& b){
        return a.hash_ == b.hash_ && a.pos_ == b.pos_ && a.size_ == b.size_;
      }

      friend bool operator!=(const Reference& a, const Reference& b){
        return !operator==(a, b);
      }
    };

    typedef std::set<Reference, Reference::PositionComparator> ReferenceSet;
   private:
    ReferenceSet references_;
   public:
    SnapshotBlockChainSection();
    SnapshotBlockChainSection(SnapshotReader* reader);
    SnapshotBlockChainSection(const SnapshotBlockChainSection& section):
      SnapshotSection(SnapshotSection::kBlockChain),
      references_(section.references_){}
    ~SnapshotBlockChainSection() = default;

    Hash GetHead() const{
      return (*references_.end()).GetHash();
    }

    int32_t GetSize() const{
      //TODO: fixme
      int32_t size = 0;
      size += Hash::kSize;
      return size;
    }

    bool Write(SnapshotWriter* writer) const;

    void operator=(const SnapshotBlockChainSection& section){
      references_ = section.references_;
    }
  };
}

#endif //TOKEN_INCLUDE_SNAPSHOT_SECTION_H