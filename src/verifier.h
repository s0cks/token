#ifndef TOKEN_VERIFIER_H
#define TOKEN_VERIFIER_H

namespace Token{
    template<typename T>
    class Verifier{
    protected:
        typedef std::vector<T> ObjectList;

        ObjectList valid_;
        ObjectList invalid_;

        Verifier():
            valid_(),
            invalid_(){}
    public:
        virtual ~Verifier() = default;

        ObjectList& valid(){
            return valid_;
        }

        ObjectList& invalid(){
            return invalid_;
        }

        bool IsValid() const{
            return invalid_.empty()
                && !valid_.empty();
        }
    };
}

#endif //TOKEN_VERIFIER_H