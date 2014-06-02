/*
 * gsmapper.h
 * 
 * Copyright 2014 gsmanners <gsmanners@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 * 
 */

#ifndef GSMAPPER_HEADER
#define GSMAPPER_HEADER

#include "irrlichttypes_extrabloated.h"
#include "client.h"
#include <map>
#include <string>
#include <vector>

class gsMapper
{
	private:
		IrrlichtDevice* d_device;
		Client* d_client;
		video::ITexture* d_pmarker;
		video::ITexture* d_txqueue[16];
		ITextureSource *d_tsrc;
		LocalPlayer* d_player;
		std::map<v3s16, u8> d_map;
		std::map<v3s16, u8> d_radar;
		std::map<std::string, u8> d_nodeids;
		std::vector<video::SColor> d_colorids;
		v3s16 d_origin;
		u16 d_posx;
		u16 d_posy;
		u16 d_width;
		u16 d_height;
		f32 d_scale;
		u32 d_alpha;
		u16 d_scan;
		s16 d_surface;
		u16 d_border;
		s16 d_scanX;
		s16 d_scanZ;
		u16 d_cooldown;
		u16 d_cooldown2;
		u16 d_texindex;
		bool d_above;
		bool d_tracking;
		bool d_valid;
		bool d_hasptex;

	public:
		gsMapper(IrrlichtDevice *device, Client *client);
		~gsMapper();
		void setMapVis(u16 x, u16 y, u16 w, u16 h, f32 scale, u32 alpha, video::SColor back);
		void setMapType(bool bAbove, u16 iScan, s16 iSurface, bool bTracking, u16 iBorder);
		void drawMap(v3s16 position);
};

#endif
