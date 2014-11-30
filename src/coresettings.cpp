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

static CoreSettings priv_core_settings;
const CoreSettings *g_core_settings = &priv_core_settings;

void CoreSettings::InitCoreSettings()
{
	priv_core_settings.init();
}


void CoreSettings::UpdateCoreSettings()
{
	priv_core_settings.update();
}


void CoreSettings::init()
{
	update();
	priv_core_settings.register_callback(g_settings, onSettingsChanged);
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

	setNeedsUpdate(false);
}


bool CoreSettings::needsUpdate() const
{
	JMutexAutoLock lock(m_mutex);
	return needs_update;
}


void CoreSettings::setNeedsUpdate(bool v)
{
	JMutexAutoLock lock(m_mutex);
	needs_update = v;
}


/* Callback function for core_settings. We don't know what triggered
 * this callback so it is kept as simple as possible. I.e. actual reloading
 * of the core settings is deferred to a point in the code where side effects
 * are more deterministic.
 */
void CoreSettings::onSettingsChanged()
{
	priv_core_settings.setNeedsUpdate(true);
}
