// Minetest
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include <exception>
#include <functional>
#include <memory>
#include "debug.h"
#include "threading/thread.h"

/**
 * Class returned by `runInThread`.
 *
 * Provides the usual thread methods along with `rethrow()`.
*/
class LambdaThread : public Thread
{
	friend std::unique_ptr<LambdaThread> runInThread(
		const std::function<void()> &, const std::string &);
public:
	/// Re-throw a caught exception, if any. Can only be called after thread exit.
	void rethrow()
	{
		sanity_check(!isRunning());
		if (m_exptr)
			std::rethrow_exception(m_exptr);
	}

private:
	// hide methods
	LambdaThread(const std::string &name="") : Thread(name) {}
	using Thread::start;

	std::function<void()> m_fn;
	std::exception_ptr m_exptr;

	void *run()
	{
		try {
			m_fn();
		} catch(...) {
			m_exptr = std::current_exception();
		}
		return nullptr;
	};
};

/**
 * Run a lambda in a separate thread.
 *
 * Exceptions will be caught.
 * @param fn function to run
 * @param thread_name name for thread
 * @return thread object of type `LambdaThread`
*/
std::unique_ptr<LambdaThread> runInThread(const std::function<void()> &fn,
	const std::string &thread_name = "")
{
	std::unique_ptr<LambdaThread> t(new LambdaThread(thread_name));
	t->m_fn = fn;
	t->start();
	return t;
}
