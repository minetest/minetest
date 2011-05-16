/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2011 Ciaran Gultnieks <ciaran@ciarang.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "servercommand.h"
#include "utility.h"

// Process a command sent from a client. The environment and connection
// should be locked when this is called.
// Returns a response message, to be dealt with according to the flags set
// in the context.
std::wstring ServerCommand::processCommand(ServerCommandContext *ctx)
{

	std::wostringstream os(std::ios_base::binary);
	ctx->flags = 1;	// Default, unless we change it.

	u64 privs = ctx->player->privs;

	if(ctx->parms.size() == 0 || ctx->parms[0] == L"help")
	{
		os<<L"-!- Available commands: ";
		os<<L"status privs ";
		if(privs & PRIV_SERVER)
			os<<L"shutdown setting ";
		if(privs & PRIV_SETTIME)
			os<<L" time";
		if(privs & PRIV_TELEPORT)
			os<<L" teleport";
		if(privs & PRIV_PRIVS)
			os<<L" grant revoke";
	}
	else if(ctx->parms[0] == L"status")
	{
		cmd_status(os, ctx);
	}
	else if(ctx->parms[0] == L"privs")
	{
		cmd_privs(os, ctx);
	}
	else if(ctx->parms[0] == L"grant" || ctx->parms[0] == L"revoke")
	{
		cmd_grantrevoke(os, ctx);
	}
	else if(ctx->parms[0] == L"time")
	{
		cmd_time(os, ctx);
	}
	else if(ctx->parms[0] == L"shutdown")
	{
		cmd_shutdown(os, ctx);
	}
	else if(ctx->parms[0] == L"setting")
	{
		cmd_setting(os, ctx);
	}
	else if(ctx->parms[0] == L"teleport")
	{
		cmd_teleport(os, ctx);
	}
	else
	{
		os<<L"-!- Invalid command: " + ctx->parms[0];
	}
	return os.str();
}

void ServerCommand::cmd_status(std::wostringstream &os,
	ServerCommandContext *ctx)
{
	os<<ctx->server->getStatusString();
}

void ServerCommand::cmd_privs(std::wostringstream &os,
	ServerCommandContext *ctx)
{
	if(ctx->parms.size() == 1)
	{
		os<<L"-!- " + privsToString(ctx->player->privs);
		return;
	}

	if((ctx->player->privs & PRIV_PRIVS) == 0)
	{
		os<<L"-!- You don't have permission to do that";
		return;
	}
		
	Player *tp = ctx->env->getPlayer(wide_to_narrow(ctx->parms[1]).c_str());
	if(tp == NULL)
	{
		os<<L"-!- No such player";
		return;
	}
	
	os<<L"-!- " + privsToString(tp->privs);
}

void ServerCommand::cmd_grantrevoke(std::wostringstream &os,
	ServerCommandContext *ctx)
{
	if(ctx->parms.size() != 3)
	{
		os<<L"-!- Missing parameter";
		return;
	}

	if((ctx->player->privs & PRIV_PRIVS) == 0)
	{
		os<<L"-!- You don't have permission to do that";
		return;
	}

	u64 newprivs = stringToPrivs(ctx->parms[2]);
	if(newprivs == PRIV_INVALID)
	{
		os<<L"-!- Invalid privileges specified";
		return;
	}

	Player *tp = ctx->env->getPlayer(wide_to_narrow(ctx->parms[1]).c_str());
	if(tp == NULL)
	{
		os<<L"-!- No such player";
		return;
	}

	if(ctx->parms[0] == L"grant")
		tp->privs |= newprivs;
	else
		tp->privs &= ~newprivs;
	
	os<<L"-!- Privileges change to ";
	os<<privsToString(tp->privs);
}

void ServerCommand::cmd_time(std::wostringstream &os,
	ServerCommandContext *ctx)
{
	if(ctx->parms.size() != 2)
	{
		os<<L"-!- Missing parameter";
		return;
	}

	if((ctx->player->privs & PRIV_SETTIME) ==0)
	{
		os<<L"-!- You don't have permission to do that";
		return;
	}

	u32 time = stoi(wide_to_narrow(ctx->parms[1]));
	ctx->server->setTimeOfDay(time);
	os<<L"-!- time_of_day changed.";
}

void ServerCommand::cmd_shutdown(std::wostringstream &os,
	ServerCommandContext *ctx)
{
	if((ctx->player->privs & PRIV_SERVER) ==0)
	{
		os<<L"-!- You don't have permission to do that";
		return;
	}

	dstream<<DTIME<<" Server: Operator requested shutdown."
		<<std::endl;
	ctx->server->requestShutdown();
					
	os<<L"*** Server shutting down (operator request)";
	ctx->flags |= 2;
}

void ServerCommand::cmd_setting(std::wostringstream &os,
	ServerCommandContext *ctx)
{
	if((ctx->player->privs & PRIV_SERVER) ==0)
	{
		os<<L"-!- You don't have permission to do that";
		return;
	}

	std::string confline = wide_to_narrow(ctx->parms[1] + L" = " + ctx->parms[2]);
	g_settings.parseConfigLine(confline);
	os<< L"-!- Setting changed.";
}

void ServerCommand::cmd_teleport(std::wostringstream &os,
	ServerCommandContext *ctx)
{
	if((ctx->player->privs & PRIV_TELEPORT) ==0)
	{
		os<<L"-!- You don't have permission to do that";
		return;
	}

	if(ctx->parms.size() != 2)
	{
		os<<L"-!- Missing parameter";
		return;
	}

	std::vector<std::wstring> coords = str_split(ctx->parms[1], L',');
	if(coords.size() != 3)
	{
		os<<L"-!- You can only specify coordinates currently";
		return;
	}

	v3f dest(stoi(coords[0])*10, stoi(coords[1])*10, stoi(coords[2])*10);
	ctx->player->setPosition(dest);
	ctx->server->SendMovePlayer(ctx->player);

	os<< L"-!- Teleported.";
}

