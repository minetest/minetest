/*
Part of Minetest-c55
Copyright (C) 2010-11 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2011 Ciaran Gultnieks <ciaran@ciarang.com>

Permission to use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "servercommand.h"
#include "utility.h"
#include "settings.h"
#include "main.h" // For g_settings
#include "content_sao.h"

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

void cmd_status(std::wostringstream &os,
	ServerCommandContext *ctx)
{
	os<<ctx->server->getStatusString();
}

void cmd_me(std::wostringstream &os,
	ServerCommandContext *ctx)
{
	std::wstring name = narrow_to_wide(ctx->player->getName());
	os << L"* " << name << L" " << ctx->paramstring;
	ctx->flags |= SEND_TO_OTHERS | SEND_NO_PREFIX;
}

void cmd_time(std::wostringstream &os,
	ServerCommandContext *ctx)
{
	if(ctx->parms.size() != 2)
	{
		os<<L"-!- Missing parameter";
		return;
	}
	
	if(!ctx->server->checkPriv(ctx->player->getName(), "settime"))
	{
		os<<L"-!- You don't have permission to do that";
		return;
	}

	u32 time = stoi(wide_to_narrow(ctx->parms[1]));
	ctx->server->setTimeOfDay(time);
	os<<L"-!- time_of_day changed.";

	actionstream<<ctx->player->getName()<<" sets time "
			<<time<<std::endl;
}

void cmd_shutdown(std::wostringstream &os,
	ServerCommandContext *ctx)
{
	if(!ctx->server->checkPriv(ctx->player->getName(), "server"))
	{
		os<<L"-!- You don't have permission to do that";
		return;
	}

	actionstream<<ctx->player->getName()
			<<" shuts down server"<<std::endl;

	ctx->server->requestShutdown();
					
	os<<L"*** Server shutting down (operator request)";
	ctx->flags |= SEND_TO_OTHERS;
}

void cmd_banunban(std::wostringstream &os, ServerCommandContext *ctx)
{
	if(!ctx->server->checkPriv(ctx->player->getName(), "ban"))
	{
		os<<L"-!- You don't have permission to do that";
		return;
	}

	if(ctx->parms.size() < 2)
	{
		std::string desc = ctx->server->getBanDescription("");
		os<<L"-!- Ban list: "<<narrow_to_wide(desc);
		return;
	}
	if(ctx->parms[0] == L"ban")
	{
		Player *player = ctx->env->getPlayer(wide_to_narrow(ctx->parms[1]).c_str());

		if(player == NULL)
		{
			os<<L"-!- No such player";
			return;
		}
		
		try{
			Address address = ctx->server->getPeerAddress(player->peer_id);
			std::string ip_string = address.serializeString();
			ctx->server->setIpBanned(ip_string, player->getName());
			os<<L"-!- Banned "<<narrow_to_wide(ip_string)<<L"|"
					<<narrow_to_wide(player->getName());

			actionstream<<ctx->player->getName()<<" bans "
					<<player->getName()<<" / "<<ip_string<<std::endl;
		} catch(con::PeerNotFoundException){
			dstream<<__FUNCTION_NAME<<": peer was not found"<<std::endl;
		}
	}
	else
	{
		std::string ip_or_name = wide_to_narrow(ctx->parms[1]);
		std::string desc = ctx->server->getBanDescription(ip_or_name);
		ctx->server->unsetIpBanned(ip_or_name);
		os<<L"-!- Unbanned "<<narrow_to_wide(desc);

		actionstream<<ctx->player->getName()<<" unbans "
				<<ip_or_name<<std::endl;
	}
}

void cmd_clearobjects(std::wostringstream &os,
	ServerCommandContext *ctx)
{
	if(!ctx->server->checkPriv(ctx->player->getName(), "server"))
	{
		os<<L"-!- You don't have permission to do that";
		return;
	}

	actionstream<<ctx->player->getName()
			<<" clears all objects"<<std::endl;
	
	{
		std::wstring msg;
		msg += L"Clearing all objects. This may take long.";
		msg += L" You may experience a timeout. (by ";
		msg += narrow_to_wide(ctx->player->getName());
		msg += L")";
		ctx->server->notifyPlayers(msg);
	}

	ctx->env->clearAllObjects();
					
	actionstream<<"object clearing done"<<std::endl;
	
	os<<L"*** cleared all objects";
	ctx->flags |= SEND_TO_OTHERS;
}


std::wstring processServerCommand(ServerCommandContext *ctx)
{

	std::wostringstream os(std::ios_base::binary);
	ctx->flags = SEND_TO_SENDER;	// Default, unless we change it.

	if(ctx->parms[0] == L"status")
		cmd_status(os, ctx);
	else if(ctx->parms[0] == L"time")
		cmd_time(os, ctx);
	else if(ctx->parms[0] == L"shutdown")
		cmd_shutdown(os, ctx);
	else if(ctx->parms[0] == L"ban" || ctx->parms[0] == L"unban")
		cmd_banunban(os, ctx);
	else if(ctx->parms[0] == L"me")
		cmd_me(os, ctx);
	else if(ctx->parms[0] == L"clearobjects")
		cmd_clearobjects(os, ctx);
	else
		os<<L"-!- Invalid command: " + ctx->parms[0];
	
	return os.str();
}


