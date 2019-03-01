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

#pragma once

#include <string>

class Gauge {
public:
	virtual void increment(const double value) = 0;
	virtual void decrement(const double value) = 0;
	virtual void set(const double value) = 0;
};

class Counter {
public:
	virtual void increment(const double value) = 0;
};

class Histogram {
	virtual void observe(const double value) = 0;
};

class Monitoring {
public:
	virtual void start() = 0;
	virtual void stop() = 0;
	virtual Gauge* createGauge(const std::string &name, const std::string &help) = 0;
	virtual Counter* createCounter(const std::string &name, const std::string &help) = 0;
	virtual Histogram* createHistogram(const std::string &name, const std::string &help, const std::vector<double> buckets) = 0;
};

// Global object
extern Monitoring* g_monitoring;
