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

#ifndef CORESETTINGS_HEADER
#define CORESETTINGS_HEADER

#include "callbackmanager.h"
#include "jthread/jmutex.h"
#include "irrlichttypes.h"

class CoreSettings : public CallbackHandler
{
public:
	CoreSettings();
	~CoreSettings();

	void init();

	void update();

	bool needsUpdate();
	void setNeedsUpdate(bool v = true);

	bool enable_shaders;
	bool enable_fog;
	bool doubletap_jump;
	bool node_highlighting;
	bool free_move;
	bool fast_move;
	bool noclip;
	bool build_where_you_stand;
	bool view_bobbing;
	bool smooth_lighting;
	bool trilinear_filter;
	bool bilinear_filter;
	bool anisotropic_filter;
	bool aux1_descends;
	bool continuous_forward;
	bool always_fly_fast;

	f32 fov;
	f32 wanted_fps;
	f32 view_bobbing_amount;
	f32 fall_bobbing_amount;

	s16 viewing_range_nodes_min;
	s16 viewing_range_nodes_max;

private:
	bool is_initialised;
	bool needs_update;

	mutable JMutex m_mutex;
};


#endif
