#include "lock.h"
#include "log.h"
#include "profiler.h"

#if !defined(NDEBUG) && !defined(LOCK_PROFILE)
//#define LOCK_PROFILE 1
#endif

#if LOCK_PROFILE
#define SCOPE_PROFILE(a) ScopeProfiler scp___(g_profiler, "Lock: " a);
#else
#define SCOPE_PROFILE(a)
#endif

template<class GUARD, class MUTEX>
recursive_lock<GUARD, MUTEX>::recursive_lock(MUTEX & mtx, std::atomic<std::size_t> & thread_id_, bool try_lock):
	thread_id(thread_id_) {
	auto thread_me = std::hash<std::thread::id>()(std::this_thread::get_id());
	if(thread_me != thread_id) {
		if (try_lock) {
			SCOPE_PROFILE("try_lock");
			lock = new GUARD(mtx, try_to_lock);
			if (lock->owns_lock()) {
				thread_id = thread_me;
				return;
			} else {
#if LOCK_PROFILE
				g_profiler->add("Lock: try_lock fail", 1);
#endif
				//infostream<<"not locked "<<" thread="<<thread_id<<" lock="<<lock<<std::endl;
			}
			delete lock;
		} else {
			SCOPE_PROFILE("lock");
			lock = new GUARD(mtx);
			thread_id = thread_me;
			return;
		}
	} else {
#if LOCK_PROFILE
		g_profiler->add("Lock: recursive", 1);
#endif
	}
	lock = nullptr;
}

template<class GUARD, class MUTEX>
recursive_lock<GUARD, MUTEX>::~recursive_lock() {
	unlock();
}

template<class GUARD, class MUTEX>
bool recursive_lock<GUARD, MUTEX>::owns_lock() {
	if (lock)
		return lock;
	auto thread_me = std::hash<std::thread::id>()(std::this_thread::get_id());
	return thread_id == thread_me;
}

template<class GUARD, class MUTEX>
void recursive_lock<GUARD, MUTEX>::unlock() {
	if(lock) {
		thread_id = 0;
		lock->unlock();
		delete lock;
		lock = nullptr;
	}
}


template<class mutex, class unique_lock, class shared_lock>
locker<mutex, unique_lock, shared_lock>::locker() {
	thread_id = 0;
}

template<class mutex, class unique_lock, class shared_lock>
std::unique_ptr<unique_lock> locker<mutex, unique_lock, shared_lock>::lock_unique() {
	return std::make_unique<unique_lock>(mtx);
}

template<class mutex, class unique_lock, class shared_lock>
std::unique_ptr<unique_lock> locker<mutex, unique_lock, shared_lock>::try_lock_unique() {
	SCOPE_PROFILE("locker::try_lock_unique");
	return std::make_unique<unique_lock>(mtx, std::try_to_lock);
}

template<class mutex, class unique_lock, class shared_lock>
std::unique_ptr<shared_lock> locker<mutex, unique_lock, shared_lock>::lock_shared() const {
	SCOPE_PROFILE("locker::lock_shared");
	return std::make_unique<shared_lock>(mtx);
}

template<class mutex, class unique_lock, class shared_lock>
std::unique_ptr<shared_lock> locker<mutex, unique_lock, shared_lock>::try_lock_shared() {
	SCOPE_PROFILE("locker::try_lock_shared");
	return std::make_unique<shared_lock>(mtx, std::try_to_lock);
}

template<class mutex, class unique_lock, class shared_lock>
std::unique_ptr<recursive_lock<unique_lock, mutex>> locker<mutex, unique_lock, shared_lock>::lock_unique_rec() const {
	SCOPE_PROFILE("locker::lock_unique_rec");
	return std::make_unique<lock_rec_unique>(mtx, thread_id);
}

template<class mutex, class unique_lock, class shared_lock>
std::unique_ptr<recursive_lock<unique_lock, mutex>> locker<mutex, unique_lock, shared_lock>::try_lock_unique_rec() {
	SCOPE_PROFILE("locker::try_lock_unique_rec");
	return std::make_unique<lock_rec_unique>(mtx, thread_id, true);
}

template<class mutex, class unique_lock, class shared_lock>
std::unique_ptr<recursive_lock<shared_lock, mutex>> locker<mutex, unique_lock, shared_lock>::lock_shared_rec() const {
	SCOPE_PROFILE("locker::lock_shared_rec");
	return std::make_unique<lock_rec_shared>(mtx, thread_id);
}

template<class mutex, class unique_lock, class shared_lock>
std::unique_ptr<recursive_lock<shared_lock, mutex>> locker<mutex, unique_lock, shared_lock>::try_lock_shared_rec() {
	SCOPE_PROFILE("locker::try_lock_shared_rec");
	return std::make_unique<lock_rec_shared>(mtx, thread_id, true);
}


template class recursive_lock<std::unique_lock<use_mutex>>;
template class locker<>;
#if LOCK_TWO
template class recursive_lock<try_shared_lock, try_shared_mutex>;
template class recursive_lock<std::unique_lock<try_shared_mutex>, try_shared_mutex>;

template class locker<try_shared_mutex, std::unique_lock<try_shared_mutex>, std::shared_lock<try_shared_mutex>>;
#endif