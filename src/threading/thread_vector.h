#pragma once

#include <atomic>
#include <thread>
#include <vector>
#include <string>

class thread_vector {
public:
	std::vector<std::thread> workers;
	std::atomic_bool request_stop;

	thread_vector(const std::string &name = "Unnamed", int priority = 0);
	virtual ~thread_vector();

	virtual void func();

	void reg (const std::string &name = "", int priority = 0);
	void start (const size_t n = 1);
	void restart (const size_t n = 1);
	void reanimate (const size_t n = 1);
	void stop ();
	void join ();

	void sleep(const int second);
// Thread compat:

	bool stopRequested();
	bool isRunning();
	void wait();
	void kill();
	virtual void * run() = 0;
	bool isCurrentThread();
protected:
	std::string m_name;
	int m_priority = 0;
};