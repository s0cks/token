#ifndef TOKEN_METRICS_H
#define TOKEN_METRICS_H

#include <map>
#include <string>
#include <memory>
#include <iostream>

namespace Token{
    namespace Metrics{
        class Metric{
        protected:
            std::string name_;

            Metric(const std::string& name):
                    name_(name){}
        public:
            virtual ~Metric() = default;

            std::string GetName() const{
                return name_;
            }
        };

        class Counter : public Metric{
        private:
            int64_t value_;
        public:
            Counter(const std::string& name, const int64_t& initial=0):
                Metric(name),
                value_(initial){}
            ~Counter() = default;

            int64_t& Get(){
                return value_;
            }

            int64_t Get() const{
                return value_;
            }

            void Set(int64_t value){
                value_ = value;
            }

            void Increment(int64_t scale=1){
                value_ += scale;
            }

            void Decrement(int64_t scale=1){
                value_ -= scale;
            }

            friend std::ostream& operator<<(std::ostream& stream, const Counter& counter){
                stream << "Counter(" << counter.Get() << ")";
                return stream;
            }
        };
    }

    typedef std::shared_ptr<Metrics::Metric> Metric;

    typedef std::shared_ptr<Metrics::Counter> Counter;
    typedef std::map<std::string, Counter> CounterMap;

    static inline Counter
    NewCounter(const std::string& name, const int64_t& initial=0){
        return std::make_shared<Metrics::Counter>(name, initial);
    }

    class MetricRegistry{
    private:
        MetricRegistry() = default;
    public:
        ~MetricRegistry() = default;
        static bool Register(const Counter& counter);
        static Counter GetCounter(const std::string& name);
    };
}

#endif //TOKEN_METRICS_H