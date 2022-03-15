#pragma once

/*
 * Mod channels callback events
 */

namespace api
{
namespace server
{
class ModChannels
{
public:
	virtual void on_modchannel_message(const std::string &channel,
			const std::string &sender, const std::string &message)
	{
	}
	virtual void on_modchannel_signal(
			const std::string &channel, ModChannelSignal signal)
	{
	}
};

}
}