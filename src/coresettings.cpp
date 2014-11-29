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

#include "coresettings.h"
#include "main.h"                // for g_settings
#include "settings.h"
#include "jthread/jmutexautolock.h"

CoreSettings::CoreSettings()
	: is_initialised(false)
{
}


CoreSettings::~CoreSettings()
{
	if (!is_initialised)
		return;

	/* Transfer values back to g_settings object so they can be saved, if
	 * required, when the program finishes.
	 */
	g_settings->setBool("enable_shaders"              , enable_shaders);
	g_settings->setBool("enable_fog"                  , enable_fog);
	g_settings->setBool("doubletap_jump"              , doubletap_jump);
	g_settings->setBool("enable_node_highlighting"    , node_highlighting);
	g_settings->setBool("free_move"                   , free_move);
	g_settings->setBool("fast_move"                   , fast_move);
	g_settings->setBool("noclip"                      , noclip);
	g_settings->setBool("enable_build_where_you_stand", build_where_you_stand);
	g_settings->setBool("view_bobbing"                , view_bobbing);
	g_settings->setBool("smooth_lighting"             , smooth_lighting);
	g_settings->setBool("trilinear_filter"            , trilinear_filter);
	g_settings->setBool("bilinear_filter"             , bilinear_filter);
	g_settings->setBool("anisotropic_filter"          , anisotropic_filter);
	g_settings->setBool("aux1_descends"               , aux1_descends);
	g_settings->setBool("continuous_forward"          , continuous_forward);
	g_settings->setBool("always_fly_fast"             , always_fly_fast);

	g_settings->setFloat("fov"                        , fov);
	g_settings->setFloat("wanted_fps"                 , wanted_fps);
	g_settings->setFloat("view_bobbing_amount"        , view_bobbing_amount);
	g_settings->setFloat("fall_bobbing_amount"        , fall_bobbing_amount);

	g_settings->setS16("viewing_range_nodes_min"      , viewing_range_nodes_min);
	g_settings->setS16("viewing_range_nodes_max"      , viewing_range_nodes_max);
}


void CoreSettings::init()
{
	update();
}


void CoreSettings::update()
{
	enable_shaders        = g_settings->getBool("enable_shaders");
	enable_fog            = g_settings->getBool("enable_fog");
	doubletap_jump        = g_settings->getBool("doubletap_jump");
	node_highlighting     = g_settings->getBool("enable_node_highlighting");
	free_move             = g_settings->getBool("free_move");
	fast_move             = g_settings->getBool("fast_move");
	noclip                = g_settings->getBool("noclip");
	build_where_you_stand = g_settings->getBool("enable_build_where_you_stand");
	view_bobbing          = g_settings->getBool("view_bobbing");
	smooth_lighting       = g_settings->getBool("smooth_lighting");
	trilinear_filter      = g_settings->getBool("trilinear_filter");
	bilinear_filter       = g_settings->getBool("bilinear_filter");
	anisotropic_filter    = g_settings->getBool("anisotropic_filter");
	aux1_descends         = g_settings->getBool("aux1_descends");
	continuous_forward    = g_settings->getBool("continuous_forward");
	always_fly_fast       = g_settings->getBool("always_fly_fast");


	fov                   = g_settings->getFloat("fov");
	wanted_fps            = g_settings->getFloat("wanted_fps");
	view_bobbing_amount   = g_settings->getFloat("view_bobbing_amount");
	fall_bobbing_amount   = g_settings->getFloat("fall_bobbing_amount");

	viewing_range_nodes_min = g_settings->getS16("viewing_range_nodes_min");
	viewing_range_nodes_max = g_settings->getS16("viewing_range_nodes_max");

	is_initialised = true;
	setNeedsUpdate(false);
}


bool CoreSettings::needsUpdate()
{
	JMutexAutoLock lock(m_mutex);
	return needs_update;
}


void CoreSettings::setNeedsUpdate(bool v)
{
	JMutexAutoLock lock(m_mutex);
	needs_update = v;
}
