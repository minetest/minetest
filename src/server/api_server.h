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
   virtual std::string formatChatMessage (
			const std::string &name, const std::string &message)
	{
		return "";
	}

   /* auth */
   virtual bool getAuth (const std::string &playername, std::string *dst_password,
         std::set<std::string> *dst_privs, s64 *dst_last_login)
   {
      return false;
   }

   virtual void createAuth (
         const std::string &playername, const std::string &password) {}
   virtual bool setPassword (
			const std::string &playername, const std::string &password)
	{
		return false;
	}

   /* dynamic media handling */
   virtual void freeDynamicMediaCallback(u32 token) {}
   virtual void on_dynamic_media_added(u32 token, const char *playername);
};
}
}