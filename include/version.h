#ifndef TOKEN_VERSION_H
#define TOKEN_VERSION_H

#include "token.h"
#include "buffer.h"

namespace Token{
    class Version{
    public:
        static const int64_t kSize = sizeof(int16_t) // Major
                                  + sizeof(int16_t) // Minor
                                  + sizeof(int16_t); // Revision
    private:
        int16_t major_;
        int16_t minor_;
        int16_t revision_;
    public:
        Version():
            major_(TOKEN_MAJOR_VERSION),
            minor_(TOKEN_MINOR_VERSION),
            revision_(TOKEN_REVISION_VERSION){}
        Version(int16_t major, int16_t minor, int16_t revision):
            major_(major),
            minor_(minor),
            revision_(revision){}
        Version(const Version& other):
            major_(other.major_),
            minor_(other.minor_),
            revision_(other.revision_){}
        Version(const Handle<Buffer>& buff):
            major_(buff->GetShort()),
            minor_(buff->GetShort()),
            revision_(buff->GetShort()){}
        ~Version() = default;

        int16_t GetMajor() const{
            return major_;
        }

        int16_t GetMinor() const{
            return minor_;
        }

        int16_t GetRevision() const{
            return revision_;
        }

        void Write(const Handle<Buffer>& buff) const{
            buff->PutShort(major_);
            buff->PutShort(minor_);
            buff->PutShort(revision_);
        }

        std::string ToString() const{
            std::stringstream ss;
            ss << GetMajor() << "." << GetMinor() << "." << GetRevision();
            return ss.str();
        }

        void operator=(const Version& other){
            major_ = other.major_;
            minor_ = other.minor_;
            revision_ = other.revision_;
        }

        bool operator==(const Version& other){
            return major_ == other.major_
                && minor_ == other.minor_
                && revision_ == other.revision_;
        }

        bool operator!=(const Version& other){
            return !operator==(other);
        }

        bool operator<(const Version& other){
            if(major_ < other.major_) return true;
            if(minor_ < other.minor_) return true;
            if(revision_ < other.minor_) return true;
            return false;
        }

        bool operator>(const Version& other){
            if(major_ > other.major_) return true;
            if(minor_ > other.minor_) return true;
            if(revision_ > other.revision_) return true;
            return false;
        }

        friend std::ostream& operator<<(std::ostream& stream, const Version& version){
            stream << version.ToString();
            return stream;
        }
    };
}

#endif //TOKEN_VERSION_H