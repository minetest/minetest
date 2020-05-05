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

#pragma once
#include <memory>
#include "config.h"
#include "util/thread.h"

class MetricCounter
{
public:
	MetricCounter() = default;

	virtual ~MetricCounter() {}

	virtual void increment(double number = 1.0) = 0;
	virtual double get() const = 0;
};

typedef std::shared_ptr<MetricCounter> MetricCounterPtr;

class SimpleMetricCounter : public MetricCounter
{
public:
	SimpleMetricCounter() = delete;

	virtual ~SimpleMetricCounter() {}

	SimpleMetricCounter(const std::string &name, const std::string &help_str) :
			MetricCounter(), m_name(name), m_help_str(help_str),
			m_counter(0.0)
	{
	}

	virtual void increment(double number)
	{
		MutexAutoLock lock(m_mutex);
		m_counter += number;
	}
	virtual double get() const
	{
		MutexAutoLock lock(m_mutex);
		return m_counter;
	}

private:
	std::string m_name;
	std::string m_help_str;

	mutable std::mutex m_mutex;
	double m_counter;
};

class MetricGauge
{
public:
	MetricGauge() = default;
	virtual ~MetricGauge() {}

	virtual void increment(double number = 1.0) = 0;
	virtual void decrement(double number = 1.0) = 0;
	virtual void set(double number) = 0;
	virtual double get() const = 0;
};

typedef std::shared_ptr<MetricGauge> MetricGaugePtr;

class SimpleMetricGauge : public MetricGauge
{
public:
	SimpleMetricGauge() = delete;

	SimpleMetricGauge(const std::string &name, const std::string &help_str) :
			MetricGauge(), m_name(name), m_help_str(help_str), m_gauge(0.0)
	{
	}

	virtual ~SimpleMetricGauge() {}

	virtual void increment(double number)
	{
		MutexAutoLock lock(m_mutex);
		m_gauge += number;
	}
	virtual void decrement(double number)
	{
		MutexAutoLock lock(m_mutex);
		m_gauge -= number;
	}
	virtual void set(double number)
	{
		MutexAutoLock lock(m_mutex);
		m_gauge = number;
	}
	virtual double get() const
	{
		MutexAutoLock lock(m_mutex);
		return m_gauge;
	}

private:
	std::string m_name;
	std::string m_help_str;

	mutable std::mutex m_mutex;
	double m_gauge;
};

class MetricsBackend
{
public:
	MetricsBackend() = default;

	virtual ~MetricsBackend() {}

	virtual MetricCounterPtr addCounter(
			const std::string &name, const std::string &help_str);
	virtual MetricGaugePtr addGauge(
			const std::string &name, const std::string &help_str);
};

#if USE_PROMETHEUS
MetricsBackend *createPrometheusMetricsBackend();
#endif
