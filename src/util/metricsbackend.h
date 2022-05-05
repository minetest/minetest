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
#include <string>
#include <utility>
#include "config.h"

class MetricCounter
{
public:
	MetricCounter() = default;

	virtual ~MetricCounter() {}

	virtual void increment(double number = 1.0) = 0;
	virtual double get() const = 0;
};

typedef std::shared_ptr<MetricCounter> MetricCounterPtr;

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

class MetricsBackend
{
public:
	MetricsBackend() = default;

	virtual ~MetricsBackend() {}

	typedef std::initializer_list<std::pair<const std::string, std::string>> Labels;

	virtual MetricCounterPtr addCounter(
			const std::string &name, const std::string &help_str,
			Labels labels = {});
	virtual MetricGaugePtr addGauge(
			const std::string &name, const std::string &help_str,
			Labels labels = {});
};

#if USE_PROMETHEUS
MetricsBackend *createPrometheusMetricsBackend();
#endif
