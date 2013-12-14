/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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


#include "server.h"
#include "hud.h"
#include "util/serialize.h"
#include "player.h"
#include "clientserver.h"

u32 Server::hudAdd(Player *player, HudElement *form) {
	if (!player)
		return -1;

	u32 id = player->getFreeHudID();
	if (id < player->hud.size())
		player->hud[id] = form;
	else
		player->hud.push_back(form);

	SendHUDAdd(player->peer_id, id, form);
	return id;
}

bool Server::hudRemove(Player *player, u32 id) {
	if (!player || id >= player->hud.size() || !player->hud[id])
		return false;

	delete player->hud[id];
	player->hud[id] = NULL;

	SendHUDRemove(player->peer_id, id);
	return true;
}

bool Server::hudChange(Player *player, u32 id, HudElementStat stat, void *data) {
	if (!player)
		return false;

	SendHUDChange(player->peer_id, id, stat, data);
	return true;
}

bool Server::hudSetFlags(Player *player, u32 flags, u32 mask) {
	if (!player)
		return false;

	SendHUDSetFlags(player->peer_id, flags, mask);
	return true;
}

bool Server::hudSetHotbarItemcount(Player *player, s32 hotbar_itemcount) {
	if (!player)
		return false;
	if (hotbar_itemcount <= 0 || hotbar_itemcount > HUD_HOTBAR_ITEMCOUNT_MAX)
		return false;

	std::ostringstream os(std::ios::binary);
	writeS32(os, hotbar_itemcount);
	SendHUDSetParam(player->peer_id, HUD_PARAM_HOTBAR_ITEMCOUNT, os.str());
	return true;
}

void Server::hudSetHotbarImage(Player *player, std::string name) {
	if (!player)
		return;

	SendHUDSetParam(player->peer_id, HUD_PARAM_HOTBAR_IMAGE, name);
}

void Server::hudSetHotbarSelectedImage(Player *player, std::string name) {
	if (!player)
		return;

	SendHUDSetParam(player->peer_id, HUD_PARAM_HOTBAR_SELECTED_IMAGE, name);
}

void Server::SendHUDAdd(u16 peer_id, u32 id, HudElement *form)
{
	std::ostringstream os(std::ios_base::binary);

	// Write command
	writeU16(os, TOCLIENT_HUDADD);
	writeU32(os, id);
	writeU8(os, (u8)form->type);
	writeV2F1000(os, form->pos);
	os << serializeString(form->name);
	writeV2F1000(os, form->scale);
	os << serializeString(form->text);
	writeU32(os, form->number);
	writeU32(os, form->item);
	writeU32(os, form->dir);
	writeV2F1000(os, form->align);
	writeV2F1000(os, form->offset);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::SendHUDRemove(u16 peer_id, u32 id)
{
	std::ostringstream os(std::ios_base::binary);

	// Write command
	writeU16(os, TOCLIENT_HUDRM);
	writeU32(os, id);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8*)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::SendHUDChange(u16 peer_id, u32 id, HudElementStat stat, void *value)
{
	std::ostringstream os(std::ios_base::binary);

	// Write command
	writeU16(os, TOCLIENT_HUDCHANGE);
	writeU32(os, id);
	writeU8(os, (u8)stat);
	switch (stat) {
		case HUD_STAT_POS:
		case HUD_STAT_SCALE:
		case HUD_STAT_ALIGN:
		case HUD_STAT_OFFSET:
			writeV2F1000(os, *(v2f *)value);
			break;
		case HUD_STAT_NAME:
		case HUD_STAT_TEXT:
			os << serializeString(*(std::string *)value);
			break;
		case HUD_STAT_NUMBER:
		case HUD_STAT_ITEM:
		case HUD_STAT_DIR:
		default:
			writeU32(os, *(u32 *)value);
			break;
	}

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8 *)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::SendHUDSetFlags(u16 peer_id, u32 flags, u32 mask)
{
	std::ostringstream os(std::ios_base::binary);

	// Write command
	writeU16(os, TOCLIENT_HUD_SET_FLAGS);
	writeU32(os, flags);
	writeU32(os, mask);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8 *)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}

void Server::SendHUDSetParam(u16 peer_id, u16 param, const std::string &value)
{
	std::ostringstream os(std::ios_base::binary);

	// Write command
	writeU16(os, TOCLIENT_HUD_SET_PARAM);
	writeU16(os, param);
	os<<serializeString(value);

	// Make data buffer
	std::string s = os.str();
	SharedBuffer<u8> data((u8 *)s.c_str(), s.size());
	// Send as reliable
	m_con.Send(peer_id, 0, data, true);
}


