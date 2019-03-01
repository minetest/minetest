/*
Minetest
Copyright (C) 2019 BuckarooBanzai/naturefreshmilk, Thomas Rudin <thomas@rudin.io>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#if ENABLE_PROMETHEUS
//provide prometheus monitoring

#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include "monitoring.h"
#include "settings.h"

class PromGauge : public Gauge {
public:
  PromGauge(prometheus::Gauge* promg){
    this->promg = promg;
  }
  void increment(const double value){
    this->promg->Increment(value);
  }
  void decrement(const double value){
    this->promg->Decrement(value);
  }
  void set(const double value){
    this->promg->Set(value);
  }
private:
  prometheus::Gauge* promg;
};


class PromCounter : public Counter {
public:
  PromCounter(prometheus::Counter* promc){
    this->promc = promc;
  }
  void increment(const double value){
    this->promc->Increment(value);
  }
private:
  prometheus::Counter* promc;
};

class PromHistogram : public Histogram {
public:
  PromHistogram(prometheus::Histogram* promh){
    this->promh = promh;
  }
  void observe(const double value){
    this->promh->Observe(value);
  }
private:
  prometheus::Histogram* promh;
};

class PrometheusMonitoring : public Monitoring {
public:
  PrometheusMonitoring(){}

  void start(){
    this->exposer = new prometheus::Exposer(g_settings->get("prometheus_bind_address"));
    this->registry = std::make_shared<prometheus::Registry>();

    this->exposer->RegisterCollectable(this->registry);
  }

  void stop(){
    delete this->exposer;
  }

  Gauge* createGauge(const std::string &name, const std::string &help){
    prometheus::Gauge& promg = prometheus::BuildGauge()
      .Name(name)
      .Help(help)
      .Register(*(this->registry))
      .Add({{"component", "main"}});

    return new PromGauge(&promg);
  }

  Counter* createCounter(const std::string &name, const std::string &help){
    prometheus::Counter& promc = prometheus::BuildCounter()
      .Name(name)
      .Help(help)
      .Register(*(this->registry))
      .Add({{"component", "main"}});

    return new PromCounter(&promc);
  }

  Histogram* createHistogram(const std::string &name, const std::string &help, const std::vector<double> buckets){
    prometheus::Histogram& promh = prometheus::BuildHistogram()
      .Name(name)
      .Help(help)
      .Register(*(this->registry))
      .Add({{"component", "main"}}, buckets);

    return new PromHistogram(&promh);
  }

private:
  prometheus::Exposer* exposer;
  std::shared_ptr<prometheus::Registry> registry;
};

static PrometheusMonitoring prom_monitoring;
Monitoring* g_monitoring = &prom_monitoring;


#else //ENABLE_PROMETHEUS
//provide no-op impl

class NoOpGauge : public Gauge {
public:
	inline void increment(const double value){}
	inline void decrement(const double value){}
	inline void set(const double value){}
};

class NoOpCouner : public Counter {
public:
  inline void increment(const double value){}
};

class NoOpHistogram : public Histogram {
public:
  inline void observe(const double value){}
};


class NoOpMonitoring : public Monitoring {
public:
	void start(){}
	void stop(){}
	Gauge* createGauge(const std::string &name, const std::string &help){
    return new NoOpGauge();
  }
  Counter* createCounter(const std::string &name, const std::string &help){
    return new NoOpCouner();
  }
  Histogram* createHistogram(const std::string &name, const std::string &help, const std::vector<double> buckets){
    return new NoOpHistogram();
  }
};

#endif //ENABLE_PROMETHEUS
