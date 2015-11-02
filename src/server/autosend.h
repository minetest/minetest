/*
Minetest
Copyright (C) 2010-2022 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#pragma once
#include "irr_v3d.h"                   // for irrlicht datatypes
#include "serverenvironment.h"
#include "emerge.h"

class RemoteClient;
struct AutosendCycle;

struct WMSSuggestion {
	WantedMapSend wms;
	bool is_fully_loaded; // Can be false for FarBlocks
	//bool is_fully_generated; // TODO
	// TODO: Using enum ServerFarBlock::LoadState would be more suitable

	WMSSuggestion():
		is_fully_loaded(true)
	{}
	WMSSuggestion(WantedMapSend wms):
		wms(wms),
		is_fully_loaded(true)
	{}
	std::string describe() const {
		if (wms.type == WMST_FARBLOCK) {
			return wms.describe()+
					": is_fully_loaded="+(is_fully_loaded?"1":"0");
		} else {
			return wms.describe();
		}
	}
};

class AutosendAlgorithm
{
public:
	AutosendAlgorithm(RemoteClient *client);
	~AutosendAlgorithm();

	// Shall be called every time before starting to ask a bunch of blocks by
	// calling getNextBlock()
	void cycle(float dtime, ServerEnvironment *env);

	// Finds a block that should be sent next to the client.
	// Environment should be locked when this is called.
	//WMSSuggestion getNextBlock(EmergeManager *emerge, ServerFarMap *far_map);
	WMSSuggestion getNextBlock(EmergeManager *emerge);

	void setParameters(s16 radius_map, s16 radius_far, float far_weight,
			float fov, u32 max_total_mapblocks, u32 max_total_farblocks) {
		m_radius_map = radius_map;
		m_radius_far = radius_far;
		m_far_weight = far_weight;
		m_fov = fov;
		m_max_total_mapblocks = max_total_mapblocks;
		m_max_total_farblocks = max_total_farblocks;
	}

	// If something is modified near a player, this should probably be called so
	// that it gets sent as quickly as possible while being prioritized
	// correctly
	void resetMapblockSearchRadius(const v3s16 &mb_p);

	std::string describeStatus();

private:
	RemoteClient *m_client;
	AutosendCycle *m_cycle;
	s16 m_radius_map; // Updated by the client; 0 disables autosend.
	s16 m_radius_far; // Updated by the client; 0 disables autosend.
	float m_far_weight; // Updated by the client.
	float m_fov; // Updated by the client; 0 disables FOV limit.
	u32 m_max_total_mapblocks;
	u32 m_max_total_farblocks;
	v3s16 m_last_focus_point;
	v3f m_last_camera_dir;
	float m_nearest_unsent_reset_timer;

	friend struct AutosendCycle;
};


