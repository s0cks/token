#ifndef TOKEN_METRICS_H
#define TOKEN_METRICS_H

#include <set>
#include <map>
#include <cmath>
#include <vector>
#include <string>
#include <memory>
#include <random>
#include <iostream>
#include <glog/logging.h>
#include <algorithm>

namespace Token{
  namespace Metrics{
#define FOR_EACH_METRIC_TYPE(V) \
        V(Counter)              \
        V(Gauge)                \
        V(Histogram)

    class Metric{
     public:
      struct NameComparator{
        bool operator()(const std::shared_ptr<Metric> &a, const std::shared_ptr<Metric> &b){
          return a->name_ < b->name_;
        }
      };
     protected:
      std::string name_;

      Metric(const std::string &name):
          name_(name){}
     public:
      virtual ~Metric() = default;

      std::string GetName() const{
        return name_;
      }

#define DEFINE_TYPE_CHECK(Name) \
            virtual bool Is##Name() const{ return false; }
      FOR_EACH_METRIC_TYPE(DEFINE_TYPE_CHECK)
#undef DEFINE_TYPE_CHECK
    };

#define DEFINE_METRIC_TYPE(Name) \
        public:                   \
            virtual bool Is##Name() const{ return true; }

    class Counter : public Metric{
     private:
      int64_t value_;
     public:
      Counter(const std::string &name, const int64_t &initial = 0):
          Metric(name),
          value_(initial){}
      ~Counter() = default;

      int64_t &Get(){
        return value_;
      }

      int64_t Get() const{
        return value_;
      }

      void Set(int64_t value){
        value_ = value;
      }

      void Increment(int64_t scale = 1){
        value_ += scale;
      }

      void Decrement(int64_t scale = 1){
        value_ -= scale;
      }

      friend std::ostream &operator<<(std::ostream &stream, const Counter &counter){
        stream << "Counter(" << counter.Get() << ")";
        return stream;
      }

     DEFINE_METRIC_TYPE(Counter);
    };

    class Gauge : public Metric{
     protected:
      Gauge(const std::string &name):
          Metric(name){}
     public:
      virtual ~Gauge() = default;
      virtual int64_t Get() const = 0;

     DEFINE_METRIC_TYPE(Gauge);
    };

    typedef std::vector<int64_t> SnapshotData;

#define FOR_EACH_PERCENTILE(V) \
        V(Median, 0.5)         \
        V(75th, 0.75)          \
        V(95th, 0.95)          \
        V(99th, 0.99)

    enum class Percentile{
#define DEFINE_PERCENTILE(Name, Quantile) k##Name##Percentile,
      FOR_EACH_PERCENTILE(DEFINE_PERCENTILE)
#undef DEFINE_PERCENTILE
    };

    static std::ostream &operator<<(std::ostream &stream, const Percentile &percentile){
      switch(percentile){
#define DEFINE_TOSTRING(Name, Quantile) \
                case Percentile::k##Name##Percentile: \
                    stream << #Name; \
                    return stream;
        FOR_EACH_PERCENTILE(DEFINE_TOSTRING)
#undef DEFINE_TOSTRING
        default:stream << "Unknown";
          return stream;
      }
    }

    static double GetQuantile(const Percentile &percentile){
      switch(percentile){
#define DEFINE_GETTER(Name, Quantile) \
                case Percentile::k##Name##Percentile: \
                    return (Quantile);
        FOR_EACH_PERCENTILE(DEFINE_GETTER)
#undef DEFINE_GETTER
        default:LOG(WARNING) << "cannot get the " << percentile << " quantile.";
          return 0;
      }
    }

    class Snapshot{
     public:
      static const int64_t kDefaultSize = 16;
     private:
      SnapshotData data_;
     public:
      Snapshot(const SnapshotData &data):
          data_(){
        std::copy(data.begin(), data.end(), std::back_inserter(data_));
        std::sort(data_.begin(), data_.end());
      }
      ~Snapshot() = default;

      bool IsEmpty() const{
        return data_.empty();
      }

      int64_t GetSize() const{
        return data_.size();
      }

      int64_t GetMin() const{
        return !IsEmpty()
               ? 0
               : data_.front();
      }

      int64_t GetMax() const{
        return !IsEmpty()
               ? 0
               : data_.back();
      }

      double GetMean() const{
        if(IsEmpty())
          return 0;
        int64_t mean = 0;
        for(auto &it : data_)
          mean += it;
        return mean / data_.size();
      }

      double GetPercentile(const Percentile &percentile) const{
        if(IsEmpty())
          return 0;

        double pos = GetQuantile(percentile) * (GetSize() + 1);
        if(pos < 1)
          return data_.front();
        else if(pos > GetSize())
          return data_.back();

        double lower = data_[pos - 1];
        double upper = data_[pos];
        return lower + (pos - std::floor(pos)) * (upper - lower);
      }

#define DEFINE_GETTER(Name, Quantile) \
            inline double Get##Name##Percentile() const{ \
                return GetPercentile(Percentile::k##Name##Percentile); \
            }
      FOR_EACH_PERCENTILE(DEFINE_GETTER)
#undef DEFINE_GETTER
    };

    class Sample{
     public:
      static const int64_t kDefaultSampleSize = 1024;
     protected:
      Sample() = default;
     public:
      virtual ~Sample() = default;
      virtual int64_t GetSize() const = 0;
      virtual void Clear() = 0;
      virtual void Update(int64_t value) = 0;
      virtual std::shared_ptr<Snapshot> GetSnapshot() const = 0;
    };

    class UniformSample : public Sample{
     private:
      int64_t size_;
      int64_t count_;
      SnapshotData data_;

      static inline int64_t
      GetRandom(int64_t count){
        std::default_random_engine engine;
        std::uniform_int_distribution<int64_t> distribution(0, count - 1);
        return distribution(engine);
      }
     public:
      UniformSample(int64_t size = Sample::kDefaultSampleSize):
          size_(size),
          count_(0),
          data_(size, 0){}
      ~UniformSample() = default;

      void Clear(){
        for(int64_t i = 0; i < size_; i++)
          data_[i] = 0;
        count_ = 0;
      }

      int64_t GetSize() const{
        int64_t size = data_.size();
        int64_t count = count_;
        return std::min(count, size);
      }

      void Update(int64_t value){
        int64_t count = count_++;
        int64_t size = data_.size();
        if(count <= size){
          data_[count - 1] = value;
        } else{
          int64_t rand = GetRandom(count);
          if(rand < size)
            data_[rand] = value;
        }
      }

      std::shared_ptr<Snapshot> GetSnapshot() const{
        return std::make_shared<Snapshot>(SnapshotData(data_));
      }
    };

    class Sampling{
     public:
      enum SampleType{
        kUniform,
        kBiased,
      };
     protected:
      Sampling() = default;
     public:
      virtual ~Sampling() = default;
      virtual std::shared_ptr<Snapshot> GetSnapshot() const = 0;
    };

    class Histogram : public Metric, Sampling{
     private:
      int64_t count_;
      std::unique_ptr<Sample> sample_;
     public:
      Histogram(const std::string &name, const Sampling::SampleType &sample_type = Sampling::kUniform):
          Metric(name),
          Sampling(),
          count_(0),
          sample_(){
        switch(sample_type){
          case Sampling::kUniform:sample_.reset(new UniformSample());
            break;
          case Sampling::kBiased:
          default:LOG(WARNING) << "unknown sample type: " << sample_type;
            return;
        }
        Clear();
      }
      ~Histogram() = default;

      std::shared_ptr<Snapshot> GetSnapshot() const{
        return sample_->GetSnapshot();
      }

      void Update(int64_t value){
        count_++;
        sample_->Update(value);
      }

      int64_t GetCount() const{
        return count_;
      }

      void Clear(){
        count_ = 0;
        sample_->Clear();
      }

     DEFINE_METRIC_TYPE(Histogram);
    };
  }

  typedef std::shared_ptr<Metrics::Metric> Metric;
  typedef std::set<Metric, Metrics::Metric::NameComparator> MetricSet;

  typedef std::shared_ptr<Metrics::Counter> Counter;
  typedef std::map<std::string, Counter> CounterMap;

  typedef std::shared_ptr<Metrics::Gauge> Gauge;
  typedef std::map<std::string, Gauge> GaugeMap;

  typedef std::shared_ptr<Metrics::Histogram> Histogram;
  typedef std::map<std::string, Histogram> HistogramMap;

  class MetricRegistryVisitor;
  class MetricRegistry{
   private:
    MetricRegistry() = default;
   public:
    ~MetricRegistry() = default;
    static bool Register(const Counter &counter);
    static bool Register(const Gauge &gauge);
    static bool Register(const Histogram &histogram);

    static bool HasCounter(const std::string &name);
    static bool HasGauge(const std::string &name);
    static bool HasHistogram(const std::string &name);

    static bool VisitMetrics(MetricRegistryVisitor *vis);
    static bool VisitCounters(MetricRegistryVisitor *vis);
    static bool VisitGauges(MetricRegistryVisitor *vis);
    static bool VisitHistograms(MetricRegistryVisitor *vis);

    static Counter GetCounter(const std::string &name);
    static Gauge GetGauge(const std::string &name);
    static Histogram GetHistogram(const std::string &name);
  };

  class MetricRegistryVisitor{
   protected:
    MetricRegistryVisitor() = default;
   public:
    virtual ~MetricRegistryVisitor() = default;
    virtual bool Visit(const Metric &metric) const = 0;
  };
}

#endif //TOKEN_METRICS_H