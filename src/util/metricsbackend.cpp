/*
Minetest
Copyright (C) 2013-2020 Minetest core developers team

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

#include "metricsbackend.h"
#include "util/thread.h"
#if USE_PROMETHEUS
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include "log.h"
#include "settings.h"
#endif

/* Plain implementation */

class SimpleMetricCounter : public MetricCounter
{
public:
	SimpleMetricCounter() : MetricCounter(), m_counter(0.0) {}

	virtual ~SimpleMetricCounter() {}

	void increment(double number) override
	{
		MutexAutoLock lock(m_mutex);
		m_counter += number;
	}
	double get() const override
	{
		MutexAutoLock lock(m_mutex);
		return m_counter;
	}

private:
	mutable std::mutex m_mutex;
	double m_counter;
};

class SimpleMetricGauge : public MetricGauge
{
public:
	SimpleMetricGauge() : MetricGauge(), m_gauge(0.0) {}

	virtual ~SimpleMetricGauge() {}

	void increment(double number) override
	{
		MutexAutoLock lock(m_mutex);
		m_gauge += number;
	}
	void decrement(double number) override
	{
		MutexAutoLock lock(m_mutex);
		m_gauge -= number;
	}
	void set(double number) override
	{
		MutexAutoLock lock(m_mutex);
		m_gauge = number;
	}
	double get() const override
	{
		MutexAutoLock lock(m_mutex);
		return m_gauge;
	}

private:
	mutable std::mutex m_mutex;
	double m_gauge;
};

MetricCounterPtr MetricsBackend::addCounter(
		const std::string &name, const std::string &help_str, Labels labels)
{
	return std::make_shared<SimpleMetricCounter>();
}

MetricGaugePtr MetricsBackend::addGauge(
		const std::string &name, const std::string &help_str, Labels labels)
{
	return std::make_shared<SimpleMetricGauge>();
}

/* Prometheus backend */

#if USE_PROMETHEUS

class PrometheusMetricCounter : public MetricCounter
{
public:
	PrometheusMetricCounter() = delete;

	PrometheusMetricCounter(const std::string &name, const std::string &help_str,
			MetricsBackend::Labels labels,
			std::shared_ptr<prometheus::Registry> registry) :
			MetricCounter(),
			m_family(prometheus::BuildCounter()
							.Name(name)
							.Help(help_str)
							.Register(*registry)),
			m_counter(m_family.Add(labels))
	{
	}

	virtual ~PrometheusMetricCounter() {}

	virtual void increment(double number) { m_counter.Increment(number); }
	virtual double get() const { return m_counter.Value(); }

private:
	prometheus::Family<prometheus::Counter> &m_family;
	prometheus::Counter &m_counter;
};

class PrometheusMetricGauge : public MetricGauge
{
public:
	PrometheusMetricGauge() = delete;

	PrometheusMetricGauge(const std::string &name, const std::string &help_str,
			MetricsBackend::Labels labels,
			std::shared_ptr<prometheus::Registry> registry) :
			MetricGauge(),
			m_family(prometheus::BuildGauge()
							.Name(name)
							.Help(help_str)
							.Register(*registry)),
			m_gauge(m_family.Add(labels))
	{
	}

	virtual ~PrometheusMetricGauge() {}

	virtual void increment(double number) { m_gauge.Increment(number); }
	virtual void decrement(double number) { m_gauge.Decrement(number); }
	virtual void set(double number) { m_gauge.Set(number); }
	virtual double get() const { return m_gauge.Value(); }

private:
	prometheus::Family<prometheus::Gauge> &m_family;
	prometheus::Gauge &m_gauge;
};

class PrometheusMetricsBackend : public MetricsBackend
{
public:
	PrometheusMetricsBackend(const std::string &addr) :
			MetricsBackend(), m_exposer(std::make_unique<prometheus::Exposer>(addr)),
			m_registry(std::make_shared<prometheus::Registry>())
	{
		m_exposer->RegisterCollectable(m_registry);
	}

	virtual ~PrometheusMetricsBackend() {}

	MetricCounterPtr addCounter(
			const std::string &name, const std::string &help_str,
			Labels labels = {}) override;
	MetricGaugePtr addGauge(
			const std::string &name, const std::string &help_str,
			Labels labels = {}) override;

private:
	std::unique_ptr<prometheus::Exposer> m_exposer;
	std::shared_ptr<prometheus::Registry> m_registry;
};

MetricCounterPtr PrometheusMetricsBackend::addCounter(
		const std::string &name, const std::string &help_str, Labels labels)
{
	return std::make_shared<PrometheusMetricCounter>(name, help_str, labels, m_registry);
}

MetricGaugePtr PrometheusMetricsBackend::addGauge(
		const std::string &name, const std::string &help_str, Labels labels)
{
	return std::make_shared<PrometheusMetricGauge>(name, help_str, labels, m_registry);
}

MetricsBackend *createPrometheusMetricsBackend()
{
	std::string addr;
	g_settings->getNoEx("prometheus_listener_address", addr);
	return new PrometheusMetricsBackend(addr);
}

#endif
