#pragma once

namespace api
{
namespace server
{
/*
 * Global server callbacks
 */

class Global
{
public:
	// Calls on_shutdown handlers
	virtual void on_shutdown() {}

	// Calls on_chat_message handlers
	// Returns true if script handled message
	virtual bool on_chat_message(const std::string &name, const std::string &message)
	{
		return false;
	}

	// Calls core.format_chat_message
	virtual std::string formatChatMessage(
			const std::string &name, const std::string &message)
	{
		return "";
	}

	/* auth */
	virtual bool getAuth(const std::string &playername, std::string *dst_password,
			std::set<std::string> *dst_privs)
	{
		return false;
	}
	virtual void createAuth(
			const std::string &playername, const std::string &password)
	{
	}
	virtual bool setPassword(
			const std::string &playername, const std::string &password)
	{
		return false;
	}
};
}
}