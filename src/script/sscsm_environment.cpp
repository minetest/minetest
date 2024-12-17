
#include "sscsm_environment.h"
#include "sscsm_requests.h"
#include "sscsm_stupid_channel.h"

SerializedSSCSMAnswer SSCSMEnvironment::exchange(SerializedSSCSMRequest req)
{
	return m_channel->exchangeA(std::move(req));
}

void SSCSMEnvironment::runEventOnStep()
{
	// example
	cmdGetNode(v3s16(0, 0, 0));
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
