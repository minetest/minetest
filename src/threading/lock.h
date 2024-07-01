/*
This file is part of Freeminer.

Freeminer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Freeminer  is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Freeminer.  If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <mutex>
#include <atomic>
#include <thread>
#include <memory>

#include "../config.h"

#ifdef _WIN32

//#include "../threading/mutex.h"
using use_mutex = std::mutex;
using try_shared_mutex = use_mutex;
using try_shared_lock = std::unique_lock<try_shared_mutex>;
using unique_lock = std::unique_lock<try_shared_mutex>;
const auto try_to_lock = std::try_to_lock;

#else

typedef std::mutex use_mutex;


#if USE_BOOST // not finished

//#include <ctime>
#include <boost/thread.hpp>
//#include <boost/thread/locks.hpp>
typedef boost::shared_mutex try_shared_mutex;
typedef boost::shared_lock<try_shared_mutex> try_shared_lock;
typedef boost::unique_lock<try_shared_mutex> unique_lock;
const auto try_to_lock = boost::try_to_lock;
#define LOCK_TWO 1

#elif HAVE_SHARED_MUTEX
//#elif __cplusplus >= 201305L

#include <shared_mutex>
using try_shared_mutex = std::shared_mutex;
using try_shared_lock = std::shared_lock<try_shared_mutex>;
using unique_lock = std::unique_lock<try_shared_mutex>;
const auto try_to_lock = std::try_to_lock;
#define LOCK_TWO 1

#else

using try_shared_mutex = use_mutex;
using try_shared_lock = std::unique_lock<try_shared_mutex> ;
using unique_lock = std::unique_lock<try_shared_mutex> ;
const auto try_to_lock = std::try_to_lock;
#endif

#endif


// http://stackoverflow.com/questions/4792449/c0x-has-no-semaphores-how-to-synchronize-threads
/* uncomment when need
#include <condition_variable>
class semaphore {
private:
	std::mutex mtx;
	std::condition_variable cv;
	int count;

public:
	semaphore(int count_ = 0):count(count_){;}
	void notify() {
	std::unique_lock<std::mutex> lck(mtx);
		++count;
		cv.notify_one();
	}
	void wait() {
		std::unique_lock<std::mutex> lck(mtx);
		while(count == 0){
			cv.wait(lck);
		}
		count--;
	}
};
*/

template<class GUARD, class MUTEX = use_mutex>
class recursive_lock {
public:
	GUARD * lock;
	std::atomic<std::size_t> & thread_id;
	recursive_lock(MUTEX & mtx, std::atomic<std::size_t> & thread_id_, bool try_lock = false);
	~recursive_lock();
	bool owns_lock();
	void unlock();
};

template<class mutex = use_mutex, class unique_lock = std::unique_lock<mutex> , class shared_lock = std::unique_lock<mutex> >
class locker {
public:
	using lock_rec_shared = recursive_lock<shared_lock, mutex>;
	using lock_rec_unique = recursive_lock<unique_lock, mutex>;

	mutable mutex mtx;
	mutable std::atomic<std::size_t> thread_id;

	locker();
	std::unique_ptr<unique_lock> lock_unique();
	std::unique_ptr<unique_lock> try_lock_unique();
	std::unique_ptr<shared_lock> lock_shared() const;
	std::unique_ptr<shared_lock> try_lock_shared();
	std::unique_ptr<lock_rec_unique> lock_unique_rec() const;
	std::unique_ptr<lock_rec_unique> try_lock_unique_rec();
	std::unique_ptr<lock_rec_shared> lock_shared_rec() const;
	std::unique_ptr<lock_rec_shared> try_lock_shared_rec();
};

using shared_locker = locker<try_shared_mutex, unique_lock, try_shared_lock>;

class dummy_lock {
public:
	~dummy_lock() {}; //no unused variable warning
	bool owns_lock() {return true;}
	bool operator!() {return true;}
	dummy_lock * operator->() {return this; }
	void unlock() {};
};

class dummy_locker {
public:
	dummy_lock lock_unique() { return {}; };
	dummy_lock try_lock_unique() { return {}; };
	dummy_lock lock_shared() { return {}; };
	dummy_lock try_lock_shared() { return {}; };
	dummy_lock lock_unique_rec() { return {}; };
	dummy_lock try_lock_unique_rec() { return {}; };
	dummy_lock lock_shared_rec() { return {}; };
	dummy_lock try_lock_shared_rec() { return {}; };
};


#if ENABLE_THREADS

using maybe_locker = locker<>;
using maybe_shared_locker = shared_locker;

#else

using maybe_locker = dummy_locker;
using maybe_shared_locker = dummy_locker;

#endif