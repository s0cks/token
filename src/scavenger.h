#ifndef TOKEN_SCAVENGER_H
#define TOKEN_SCAVENGER_H

namespace Token{
    class Scavenger{
    private:
        bool is_major_;

        Scavenger(bool is_major):
            is_major_(is_major){}

        bool ScavengeMemory();
    public:
        ~Scavenger() = default;

        static bool Scavenge(bool is_major){
            Scavenger scavenger(is_major);
            return scavenger.ScavengeMemory();
        }
    };
}

#endif //TOKEN_SCAVENGER_H