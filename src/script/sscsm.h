
#pragma once

#include <memory>
#include <type_traits>
#include "irrlichttypes.h"
#include "client/clientenvironment.h"
#include "map.h"

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
SerializedSSCSMAnswer serializedSSCSMAnswer(const T &answer)
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


struct SSCSMEnvironment;

struct SSCSMControler
{
	// FIXME: replace this with an ipc channel
	SSCSMEnvironment *m_sscsm_env;
	// Note: Not needed later. Instead of us passing control flow to the SSCSMEnvironment
	// via run() (see event*() member functions), the other process will already
	// be running (or rather waiting), and we only need to "return" the next event.
	std::optional<int> m_next_event;

	ClientEnvironment *m_clientenv;

	SerializedSSCSMAnswer handleRequest(ISSCSMRequest *req)
	{
		return req->exec(this);
	}

	void eventOnStep(f32 dtime);
};

struct SSCSMEnvironment
{
	// FIXME: replace this with an ipc channel
	SSCSMControler *m_channel;

	void run()
	{
		// Note: This will later run in another process, and only return if it
		// wants to tear down the process. The next event will then also no
		// longer be optional.

		while (true) {
			auto next_event = cmdPollNextEvent();
			if (!next_event.has_value())
				break;

			if (*next_event == 42)
				runEventOnStep();
		}
	}

	SerializedSSCSMAnswer exchange(const SerializedSSCSMRequest &req)
	{
		return m_channel->handleRequest(req.get());
	}

	void runEventOnStep()
	{
	}

	std::optional<int> cmdPollNextEvent();
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

		return serializedSSCSMAnswer(SSCSMAnswerGetNode{node});
	}
};

struct SSCSMAnswerPollNextEvent : public ISSCSMAnswer
{
	std::optional<int> next_event;

	SSCSMAnswerPollNextEvent(std::optional<int> next_event_) : next_event(next_event_) {}
};

struct SSCSMRequestPollNextEvent : public ISSCSMRequest
{
	virtual SerializedSSCSMAnswer exec(SSCSMControler *cntrl)
	{
		auto next_event = cntrl->m_next_event;
		cntrl->m_next_event = std::nullopt;

		return serializedSSCSMAnswer(SSCSMAnswerPollNextEvent{next_event});
	}
};


void SSCSMControler::eventOnStep(f32 dtime)
{
	m_next_event = 42;

	m_sscsm_env->run();
}



std::optional<int> SSCSMEnvironment::cmdPollNextEvent()
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
