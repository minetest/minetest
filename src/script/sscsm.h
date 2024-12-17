
#pragma once

#include <mutex>
#include <condition_variable>
#include <memory>
#include <type_traits>
#include "irrlichttypes.h"
#include "client/clientenvironment.h"
#include "map.h"
#include "threading/thread.h"

struct SSCSMControler;

struct ISSCSMAnswer
{
	virtual ~ISSCSMAnswer() = default;
};

// FIXME: actually serialize, and replace this by a string
using SerializedSSCSMAnswer = std::unique_ptr<ISSCSMAnswer>;

struct ISSCSMRequest
{
	virtual ~ISSCSMRequest() = default;

	virtual SerializedSSCSMAnswer exec(SSCSMControler *cntrl) = 0;
};

// FIXME: actually serialize, and replace this by a string
using SerializedSSCSMRequest = std::unique_ptr<ISSCSMRequest>;

template <typename T>
SerializedSSCSMRequest serializeSSCSMRequest(const T &request)
{
	static_assert(std::is_base_of_v<ISSCSMRequest, T>);

	return std::make_unique<T>(request);
}

template <typename T>
T deserializeSSCSMAnswer(const SerializedSSCSMAnswer &answer_serialized)
{
	static_assert(std::is_base_of_v<ISSCSMAnswer, T>);

	// dynamic cast in place of actual deserialization
	auto ptr = dynamic_cast<const T *>(answer_serialized.get());
	if (!ptr) {
		throw 0; //TODO: serialization excpetion
	}
	return *ptr;
}

template <typename T>
SerializedSSCSMAnswer serializeSSCSMAnswer(const T &answer)
{
	static_assert(std::is_base_of_v<ISSCSMAnswer, T>);

	return std::make_unique<T>(answer);
}

std::unique_ptr<ISSCSMRequest> deserializeSSCSMRequest(SerializedSSCSMRequest request_serialized)
{
	// The actual deserialization will have to use a type tag, and then choose
	// the appropriate deserializer.
	return request_serialized;
}


// FIXME: replace this with an ipc channel
class StupidChannel
{
	std::mutex m_mutex;
	std::condition_variable m_condvar;
	SerializedSSCSMRequest m_request;
	SerializedSSCSMAnswer m_answer;

public:
	void sendA(SerializedSSCSMRequest request)
	{
		{
			auto lock = std::lock_guard(m_mutex);

			m_request = std::move(request);
		}

		m_condvar.notify_one();
	}

	SerializedSSCSMAnswer recvA()
	{
		auto lock = std::unique_lock(m_mutex);

		while (!m_answer) {
			m_condvar.wait(lock);
		}

		auto answer = std::move(m_answer);
		m_answer = nullptr;

		return answer;
	}

	SerializedSSCSMAnswer exchangeA(SerializedSSCSMRequest request)
	{
		sendA(std::move(request));

		return recvA();
	}

	void sendB(SerializedSSCSMAnswer answer)
	{
		{
			auto lock = std::lock_guard(m_mutex);

			m_answer = std::move(answer);
		}

		m_condvar.notify_one();
	}

	SerializedSSCSMRequest recvB()
	{
		auto lock = std::unique_lock(m_mutex);

		while (!m_request) {
			m_condvar.wait(lock);
		}

		auto request = std::move(m_request);
		m_request = nullptr;

		return request;
	}

	SerializedSSCSMRequest exchangeB(SerializedSSCSMAnswer answer)
	{
		sendB(std::move(answer));

		return recvB();
	}
};


struct SSCSMEnvironment;

struct SSCSMControler
{
	std::unique_ptr<SSCSMEnvironment> m_thread;
	std::shared_ptr<StupidChannel> m_channel;
	ClientEnvironment *m_clientenv;

	SSCSMControler(std::unique_ptr<SSCSMEnvironment> thread,
			std::shared_ptr<StupidChannel> channel,
			ClientEnvironment *clientenv) :
		m_thread(std::move(thread)), m_channel(std::move(channel)),
		m_clientenv(clientenv)
	{
	}

	static std::unique_ptr<SSCSMControler> create(ClientEnvironment *clientenv);

	SerializedSSCSMAnswer handleRequest(ISSCSMRequest *req)
	{
		return req->exec(this);
	}

	// Handles requests until the next event is polled
	void runEvent(int event);

	void eventTearDown();
	void eventOnStep(f32 dtime);
};

struct SSCSMEnvironment : Thread
{
	std::shared_ptr<StupidChannel> m_channel;

	SSCSMEnvironment(std::shared_ptr<StupidChannel> channel) :
		Thread("SSCSMEnvironment-thread"),
		m_channel(std::move(channel))
	{
	}

	void *run()
	{
		while (true) {
			auto next_event = cmdPollNextEvent();

			if (next_event == 0) // tear down
				break;

			if (next_event == 42)
				runEventOnStep();
		}

		return nullptr;
	}

	SerializedSSCSMAnswer exchange(SerializedSSCSMRequest req)
	{
		return m_channel->exchangeA(std::move(req));
	}

	void runEventOnStep()
	{
		cmdGetNode(v3s16(0, 0, 0));
	}

	int cmdPollNextEventNoRequest();
	int cmdPollNextEvent();
	MapNode cmdGetNode(v3s16 pos);
};


struct SSCSMAnswerGetNode : public ISSCSMAnswer
{
	MapNode node;

	SSCSMAnswerGetNode(MapNode node_) : node(node_) {}
};

struct SSCSMRequestGetNode : public ISSCSMRequest
{
	v3s16 pos;

	SSCSMRequestGetNode(v3s16 pos_) : pos(pos_) {}

	virtual SerializedSSCSMAnswer exec(SSCSMControler *cntrl)
	{
		MapNode node = cntrl->m_clientenv->getMap().getNode(pos);

		return serializeSSCSMAnswer(SSCSMAnswerGetNode{node});
	}
};

struct SSCSMAnswerPollNextEvent : public ISSCSMAnswer
{
	int next_event;

	SSCSMAnswerPollNextEvent(int next_event_) : next_event(next_event_) {}
};

struct SSCSMRequestPollNextEvent : public ISSCSMRequest
{
	virtual SerializedSSCSMAnswer exec(SSCSMControler *cntrl)
	{
		FATAL_ERROR("SSCSMRequestPollNextEvent needs to be handled by SSCSMControler::runEvent()");
	}
};



std::unique_ptr<SSCSMControler> SSCSMControler::create(ClientEnvironment *clientenv)
{
	auto channel = std::make_shared<StupidChannel>();
	auto thread = std::make_unique<SSCSMEnvironment>(channel);

	// Wait for thread to finish initializing.
	auto req0 = deserializeSSCSMRequest(channel->recvB());
	FATAL_ERROR_IF(!dynamic_cast<SSCSMRequestPollNextEvent *>(req0.get()),
			"First request must be pollEvent.");

	return std::make_unique<SSCSMControler>(std::move(thread), channel, clientenv);
}

void SSCSMControler::runEvent(int event)
{
	auto answer = serializeSSCSMAnswer(SSCSMAnswerPollNextEvent{event});

	while (true) {
		auto request = deserializeSSCSMRequest(m_channel->exchangeB(std::move(answer)));

		if (dynamic_cast<SSCSMRequestPollNextEvent *>(request.get()) != nullptr) {
			break;
		}

		answer = handleRequest(request.get());
	}
}

void SSCSMControler::eventTearDown()
{
	runEvent(0);
}

void SSCSMControler::eventOnStep(f32 dtime)
{
	runEvent(42);
}



int SSCSMEnvironment::cmdPollNextEventNoRequest()
{
	auto answer = deserializeSSCSMAnswer<SSCSMAnswerPollNextEvent>(
			m_channel->recvA()
		);
	return answer.next_event;
}

int SSCSMEnvironment::cmdPollNextEvent()
{
	auto request = SSCSMRequestPollNextEvent{};
	auto answer = deserializeSSCSMAnswer<SSCSMAnswerPollNextEvent>(
			exchange(serializeSSCSMRequest(request))
		);
	return answer.next_event;
}

MapNode SSCSMEnvironment::cmdGetNode(v3s16 pos)
{
	auto request = SSCSMRequestGetNode{pos};
	auto answer = deserializeSSCSMAnswer<SSCSMAnswerGetNode>(
			exchange(serializeSSCSMRequest(request))
		);
	return answer.node;
}
