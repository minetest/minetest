/*
Minetest
Copyright (C) 2017 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "s_modchannels.h"
#include "s_internal.h"

void ScriptApiModChannels::on_modchannel_message(const std::string &channel,
		const std::string &sender, const std::string &message)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_generateds
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_modchannel_message");
	// Call callbacks
	lua_pushstring(L, channel.c_str());
	lua_pushstring(L, sender.c_str());
	lua_pushstring(L, message.c_str());
	runCallbacks(3, RUN_CALLBACKS_MODE_AND);
}

void ScriptApiModChannels::on_modchannel_signal(
		const std::string &channel, ModChannelSignal signal)
{
	SCRIPTAPI_PRECHECKHEADER

	// Get core.registered_on_generateds
	lua_getglobal(L, "core");
	lua_getfield(L, -1, "registered_on_modchannel_signal");
	// Call callbacks
	lua_pushstring(L, channel.c_str());
	lua_pushinteger(L, (int)signal);
	runCallbacks(2, RUN_CALLBACKS_MODE_AND);
}
