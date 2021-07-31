#ifndef TKN_BUILDER_H
#define TKN_BUILDER_H

namespace token{
  namespace internal{
    template<class T>
    class BuilderBase{
    protected:
      BuilderBase() = default;
    public:
      virtual ~BuilderBase() = default;
      virtual std::shared_ptr<T> Build() const = 0;
    };

    template<class T, class P>
    class ProtoBuilder : public BuilderBase<T>{
    protected:
      P* raw_;
      bool free_;

      explicit ProtoBuilder(P* raw):
        BuilderBase<T>(),
        raw_(raw),
        free_(false){}
      ProtoBuilder():
        BuilderBase<T>(),
        raw_(new P()),
        free_(true){}
    public:
      ~ProtoBuilder() override{
        if(free_ && raw_)
          delete raw_;
      }

      inline const P*
      raw() const{
        return raw_;
      }

      std::shared_ptr<T> Build() const override = 0;
    };
  }
}

#endif//TKN_BUILDER_H