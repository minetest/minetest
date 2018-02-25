#ifndef _SOUND_ESPEAKNG_H_
#define _SOUND_ESPEAKNG_H_

#include <condition_variable>
#include <deque>
#include <exception>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

/* FIXME: inclusion macro duplicated from sound_openal.cpp - unify these */
#if defined(_WIN32)
	#include <al.h>
	#include <alc.h>
	//#include <alext.h>
#elif defined(__APPLE__)
	#include <OpenAL/al.h>
	#include <OpenAL/alc.h>
	//#include <OpenAL/alext.h>
#else
	#include <AL/al.h>
	#include <AL/alc.h>
	#include <AL/alext.h>
#endif

#include <sound_tts_manager.h>

typedef ::std::unique_ptr<ALuint, void(*)(ALuint *p)> unique_ptr_alsource;
typedef ::std::unique_ptr<ALuint, void(*)(ALuint *p)> unique_ptr_albuffer;

class MtESpeak
{
public:
	MtESpeak();
	~MtESpeak();

	void start();
	void join();

	void requestEnqueue(MtESpeakRequest req);

	void threadFunc();
	void threadFunc2();

	void maintain();

public:
	std::exception_ptr m_thread_exc;
private:
	std::thread m_thread;
	std::mutex                  m_mutex;
	std::condition_variable     m_request_queue_cv;
	std::deque<MtESpeakRequest> m_request_queue;

	std::string m_data_path;
	unique_ptr_alsource m_source;
	size_t m_sample_rate;
};

#endif /* _SOUND_ESPEAKNG_H_ */
