#ifndef TOKEN_VERSION_H
#define TOKEN_VERSION_H

#include "utils/bitfield.h"

namespace token{
  typedef uint64_t RawVersion;

  class Version{
   public:
    static inline int
    Compare(const Version& a, const Version& b){
      if(!a.valid())
        LOG(WARNING) << "a is not valid. (magic=" << std::hex << MagicField::Decode(a.raw()) << ")";

      if(!b.valid())
        LOG(WARNING) << "b is not valid. (magic=" << std::hex << MagicField::Decode(b.raw()) << ")";

      if(a.major() < b.major()){
        return -1;
      } else if(a.major() > b.major()){
        return +1;
      }

      if(a.minor() < b.minor()){
        return -1;
      } else if(a.minor() > b.minor()){
        return +1;
      }

      if(a.revision() < b.revision()){
        return -1;
      } else if(a.revision() > b.revision()){
        return +1;
      }
      return 0;
    }
   private:
    enum Layout{
      kMagicPosition = 0,
      kBitsForMagic = 16,

      kMajorPosition = kMagicPosition+kBitsForMagic,
      kBitsForMajor = 16,

      kMinorPosition = kMajorPosition+kBitsForMajor,
      kBitsForMinor = 16,

      kRevisionPosition = kMinorPosition+kBitsForMinor,
      kBitsForRevision = 16,

      kTotalSize = kRevisionPosition+kBitsForRevision,
    };

    RawVersion raw_;

    class MagicField : public BitField<RawVersion, uint16_t, kMagicPosition, kBitsForMagic>{};
    class MajorField : public BitField<RawVersion, uint16_t, kMajorPosition, kBitsForMajor>{};
    class MinorField : public BitField<RawVersion, uint16_t, kMinorPosition, kBitsForMinor>{};
    class RevisionField : public BitField<RawVersion, uint16_t, kRevisionPosition, kBitsForRevision>{};
   public:
    Version(const RawVersion& raw):
      raw_(raw){}
    Version(const uint16_t major, const uint16_t minor, const uint16_t revision):
      raw_(MagicField::Encode(TOKEN_MAGIC)
          |MajorField::Encode(major)
          |MinorField::Encode(minor)
          |RevisionField::Encode(revision)){}
    Version(const Version& version):
      raw_(version.raw_){}
    ~Version() = default;

    RawVersion raw() const{
      return raw_;
    }

    uint16_t magic() const{
      return MagicField::Decode(raw_);
    }

    uint16_t major() const{
      return MajorField::Decode(raw_);
    }

    uint16_t minor() const{
      return MinorField::Decode(raw_);
    }

    uint16_t revision() const{
      return RevisionField::Decode(raw_);
    }

    bool valid() const{
      return magic() == TOKEN_MAGIC;
    }

    std::string ToString() const{
      std::stringstream ss;
      ss << major() << '.' << minor() << '.' << revision();
      return ss.str();
    }

    Version& operator=(const Version& version){
      raw_ = version.raw_;
      return (*this);
    }

    friend bool operator==(const Version& a, const Version& b){
      return Compare(a, b) == 0;
    }

    friend bool operator!=(const Version& a, const Version& b){
      return Compare(a, b) != 0;
    }

    friend bool operator<(const Version& a, const Version& b){
      return Compare(a, b) < 0;
    }

    friend bool operator>(const Version& a, const Version& b){
      return Compare(a, b) > 0;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Version& version){
      return stream << version.ToString();
    }
  };
}

#endif //TOKEN_VERSION_H