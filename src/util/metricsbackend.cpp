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

MetricPtr MetricsBackend::addCounter(const std::string &name, const std::string &help_str,
		const MetricLabels &labels)
{
	return std::make_shared<SimpleMetricCounter>(name, help_str);
}

MetricGaugePtr MetricsBackend::addGauge(const std::string &name,
		const std::string &help_str, const MetricLabels &labels)
{
	return std::make_shared<SimpleMetricGauge>(name, help_str);
}

#if USE_PROMETHEUS

class PrometheusMetricCounter : public Metric
{
public:
	PrometheusMetricCounter() = delete;
	PrometheusMetricCounter(prometheus::Counter &counter) : counter(counter) {}
	~PrometheusMetricCounter() override {}

	void increment(double number) override { counter.Increment(number); }
	double get() const override { return counter.Value(); }

private:
	prometheus::Counter &counter;
};

class PrometheusMetricGauge : public MetricGauge
{
public:
	PrometheusMetricGauge() = delete;
	PrometheusMetricGauge(prometheus::Gauge &gauge) : gauge(gauge) {}
	~PrometheusMetricGauge() override {}

	void increment(double number) override { gauge.Increment(number); }
	void decrement(double number) override { gauge.Decrement(number); }
	void set(double number) override { gauge.Set(number); }
	double get() const override { return gauge.Value(); }

private:
	prometheus::Gauge &gauge;
};

class PrometheusMetricsBackend : public MetricsBackend
{
	std::unordered_map<std::string, prometheus::Family<prometheus::Counter> &>
			counter_families;
	std::unordered_map<std::string, prometheus::Family<prometheus::Gauge> &>
			gauge_families;

	prometheus::Family<prometheus::Counter> &getCreateCounterFamily(
			const std::string &name, const std::string &help_str)
	{
		auto it = counter_families.find(name);
		if (it != counter_families.end()) {
			return it->second;
		}

		auto &family = prometheus::BuildCounter()
					       .Name(name)
					       .Help(help_str)
					       .Register(*m_registry);

		counter_families.emplace(name, family);
		return family;
	}

	prometheus::Family<prometheus::Gauge> &getCreateGaugeFamily(
			const std::string &name, const std::string &help_str)
	{
		auto it = gauge_families.find(name);
		if (it != gauge_families.end()) {
			return it->second;
		}

		auto &family = prometheus::BuildGauge()
					       .Name(name)
					       .Help(help_str)
					       .Register(*m_registry);

		gauge_families.emplace(name, family);
		return family;
	}

public:
	PrometheusMetricsBackend(const std::string &addr) :
			m_exposer(std::unique_ptr<prometheus::Exposer>(
					new prometheus::Exposer(addr))),
			m_registry(std::make_shared<prometheus::Registry>())
	{
		m_exposer->RegisterCollectable(m_registry);
	}

	~PrometheusMetricsBackend() override {}

	MetricPtr addCounter(const std::string &name, const std::string &help_str,
			const MetricLabels &labels) override
	{
		auto &family = getCreateCounterFamily(name, help_str);
		return std::make_shared<PrometheusMetricCounter>(family.Add(labels));
	}

	MetricGaugePtr addGauge(const std::string &name, const std::string &help_str,
			const MetricLabels &labels) override
	{
		auto &family = getCreateGaugeFamily(name, help_str);
		return std::make_shared<PrometheusMetricGauge>(family.Add(labels));
	}

private:
	std::unique_ptr<prometheus::Exposer> m_exposer;
	std::shared_ptr<prometheus::Registry> m_registry;
};

MetricsBackend *createPrometheusMetricsBackend()
{
	std::string addr;
	g_settings->getNoEx("prometheus_listener_address", addr);
	return new PrometheusMetricsBackend(addr);
}

#endif
