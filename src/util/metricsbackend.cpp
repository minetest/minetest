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
#if USE_PROMETHEUS
#include <prometheus/exposer.h>
#include <prometheus/registry.h>
#include <prometheus/counter.h>
#include <prometheus/gauge.h>
#include "log.h"
#include "settings.h"
#endif

MetricCounterPtr MetricsBackend::addCounter(
		const std::string &name, const std::string &help_str)
{
	return std::make_shared<SimpleMetricCounter>(name, help_str);
}

MetricGaugePtr MetricsBackend::addGauge(
		const std::string &name, const std::string &help_str)
{
	return std::make_shared<SimpleMetricGauge>(name, help_str);
}

#if USE_PROMETHEUS

class PrometheusMetricCounter : public MetricCounter
{
public:
	PrometheusMetricCounter() = delete;

	PrometheusMetricCounter(const std::string &name, const std::string &help_str,
			std::shared_ptr<prometheus::Registry> registry) :
			MetricCounter(),
			m_family(prometheus::BuildCounter()
							.Name(name)
							.Help(help_str)
							.Register(*registry)),
			m_counter(m_family.Add({}))
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
			std::shared_ptr<prometheus::Registry> registry) :
			MetricGauge(),
			m_family(prometheus::BuildGauge()
							.Name(name)
							.Help(help_str)
							.Register(*registry)),
			m_gauge(m_family.Add({}))
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
			MetricsBackend(), m_exposer(std::unique_ptr<prometheus::Exposer>(
							  new prometheus::Exposer(addr))),
			m_registry(std::make_shared<prometheus::Registry>())
	{
		m_exposer->RegisterCollectable(m_registry);
	}

	virtual ~PrometheusMetricsBackend() {}

	virtual MetricCounterPtr addCounter(
			const std::string &name, const std::string &help_str);
	virtual MetricGaugePtr addGauge(
			const std::string &name, const std::string &help_str);

private:
	std::unique_ptr<prometheus::Exposer> m_exposer;
	std::shared_ptr<prometheus::Registry> m_registry;
};

MetricCounterPtr PrometheusMetricsBackend::addCounter(
		const std::string &name, const std::string &help_str)
{
	return std::make_shared<PrometheusMetricCounter>(name, help_str, m_registry);
}

MetricGaugePtr PrometheusMetricsBackend::addGauge(
		const std::string &name, const std::string &help_str)
{
	return std::make_shared<PrometheusMetricGauge>(name, help_str, m_registry);
}

MetricsBackend *createPrometheusMetricsBackend()
{
	std::string addr;
	g_settings->getNoEx("prometheus_listener_address", addr);
	return new PrometheusMetricsBackend(addr);
}

#endif
