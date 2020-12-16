#ifndef TOKEN_METRICS_H
#define TOKEN_METRICS_H

#include <map>
#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include <algorithm>

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

        class Gauge : public Metric{
        protected:
            Gauge(const std::string& name):
                Metric(name){}
        public:
            virtual ~Gauge() = default;
            virtual int64_t Get() const = 0;
        };
    }

    typedef std::shared_ptr<Metrics::Metric> Metric;


    typedef std::shared_ptr<Metrics::Counter> Counter;
    typedef std::map<std::string, Counter> CounterMap;

    typedef std::shared_ptr<Metrics::Gauge> Gauge;
    typedef std::map<std::string, Gauge> GaugeMap;

    class MetricRegistry{
    private:
        MetricRegistry() = default;
    public:
        ~MetricRegistry() = default;
        static bool Register(const Counter& counter);
        static bool Register(const Gauge& gauge);
        static bool HasCounter(const std::string& name);
        static bool HasGauge(const std::string& name);
        static Counter GetCounter(const std::string& name);
        static Gauge GetGauge(const std::string& name);
    };
}

#endif //TOKEN_METRICS_H