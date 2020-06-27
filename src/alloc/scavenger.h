#ifndef TOKEN_SCAVENGER_H
#define TOKEN_SCAVENGER_H

#include "raw_object.h"

namespace Token{
    class Marker{
    private:
        Color color_;
    public:
        Marker(Color color):
            color_(color){}
        ~Marker(){}

        void MarkObject(RawObject* obj){
            LOG(INFO) << "marking object: " << (*obj);
            obj->SetColor(color_);
        }

        Color GetColor() const{
            return color_;
        }

        Marker& operator=(const Marker& other){
            color_ = other.color_;
            return (*this);
        }
    };

    class Scavenger{
    protected:
        bool DarkenRoots();
        bool MarkObjects();
        virtual bool ScavengeMemory() = 0;
    public:
        virtual ~Scavenger() = default;
    };
}

#endif //TOKEN_SCAVENGER_H