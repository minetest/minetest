/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "settings.h"
#include "porting.h"
#include "filesys.h"
#include "config.h"
#include "constants.h"
#include "porting.h"
#include "util/string.h"

void set_default_settings(Settings *settings)
{
#define MTCONF_SET_EXT(enumname, lowercase, default) settings->setDefault(lowercase, default);
#include "settingdef.h"
#undef MTCONF_SET_EXT

#ifdef __ANDROID__
	settings->setDefault("screenW", "0");
	settings->setDefault("screenH", "0");
	settings->setDefault("enable_shaders", "false");
	settings->setDefault("fullscreen", "true");
	settings->setDefault("enable_particles", "false");
	settings->setDefault("video_driver", "ogles1");
	settings->setDefault("smooth_lighting", "false");
	settings->setDefault("max_simultaneous_block_sends_per_client", "3");
	settings->setDefault("emergequeue_limit_diskonly", "8");
	settings->setDefault("emergequeue_limit_generate", "8");

	settings->setDefault("viewing_range_nodes_max", "50");
	settings->setDefault("viewing_range_nodes_min", "20");

	//check for device with small screen
	float x_inches = ((double) porting::getDisplaySize().X /
			(160 * porting::getDisplayDensity()));
	if (x_inches  < 3.5) {
		settings->setDefault("hud_scaling", "0.6");
	} else if (x_inches < 4.5) {
		settings->setDefault("hud_scaling", "0.7");
	}
	settings->setDefault("curl_verify_cert", "false");
#endif
}

void override_default_settings(Settings *settings, Settings *from)
{
	std::vector<std::string> names = from->getNames();
	for(size_t i=0; i<names.size(); i++){
		const std::string &name = names[i];
		settings->setDefault(name, from->get(name));
	}
}
