/*
 * gsmapper.cpp
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

#include "gsmapper.h"
#include "map.h"
#include "nodedef.h"
#include "tile.h"
#include "util/numeric.h"
#include "util/string.h"
#include <math.h>

gsMapper::gsMapper(IrrlichtDevice *device, Client *client):
	d_device(device),
	d_client(client)
{
	d_colorids.clear();
	d_nodeids.clear();

	d_colorids.push_back(video::SColor(0,0,0,0));		// 0 = background (air)
	d_nodeids[CONTENT_AIR] = 0;
	d_colorids.push_back(video::SColor(0,0,0,0));		// 1 = unloaded/error node
	d_nodeids[CONTENT_IGNORE] = 1;
	d_colorids.push_back(video::SColor(0,99,99,99));	// 2 = mystery node
	d_nodeids[CONTENT_UNKNOWN] = 2;
	d_colordefs = 3;

	d_valid = false;
	d_hastex = false;
	d_hasptex = false;
	d_scanX = 0;
	d_scanZ = 0;
	d_cooldown2 = 0;
	d_map.clear();
	d_radar.clear();

	d_tsrc = d_client->getTextureSource();
	d_player = d_client->getEnv().getLocalPlayer();
}

gsMapper::~gsMapper()
{
	video::IVideoDriver* driver = d_device->getVideoDriver();
	if (d_hastex)
	{
		driver->removeTexture(d_texture);
		d_hastex = false;
	}
	if (d_hasptex)
	{
		driver->removeTexture(d_pmarker);
		d_hasptex = false;
	}
}

/*
 * Convert the content id to a color value.
 *
 * Any time we encounter a node that isn't in the ids table, this converts the
 * texture of the node into a color. Otherwise, this returns what we already converted.
 */

video::SColor gsMapper::getColorFromId(u16 id)
{
	// check if already in my defs
	std::map<u16, u16>::iterator i = d_nodeids.find(id);
	if (i != d_nodeids.end()) {
		return d_colorids[d_nodeids[id]];

	} else {

		// get the tile image
		const ContentFeatures &f = d_client->getNodeDefManager()->get(id);
		video::ITexture *t = d_tsrc->getTexture(f.tiledef[0].name);
		assert(t);

		video::IVideoDriver *driver = d_device->getVideoDriver();

		video::IImage *image = driver->createImage(t, 
			core::position2d<s32>(0, 0),
			t->getOriginalSize());
		assert(image);

		// copy to a 1-pixel image
		video::IImage *image2 = driver->createImage(video::ECF_A8R8G8B8,
			core::dimension2d<u32>(1, 1) );
		assert(image2);
		image->copyToScalingBoxFilter(image2, 0, false);
		image->drop();

		// add the color to my list
		d_colorids.push_back(image2->getPixel(0,0));
		image2->drop();
		d_nodeids[id] = d_colordefs;
		assert(d_colordefs < MAX_REGISTERED_CONTENT);
		return d_colorids[d_colordefs++];
	}
}

/*
 * Set the visual elements of the HUD (rectangle) on the display.
 *
 * x, y = screen coordinates of the HUD
 * w, h = width and height of the HUD
 * scale = pixel scale (pixels per node)
 * alpha = alpha component of the HUD
 * back = background color
 * 
 * Note that the scaling is not currently interpolated.
 */

void gsMapper::setMapVis(u16 x, u16 y, u16 w, u16 h, f32 scale, u32 alpha, video::SColor back)
{
	if (x != d_posx)
	{
		d_posx = x;
		d_valid = false;
	}
	if (y != d_posy)
	{
		d_posy = y;
		d_valid = false;
	}
	if (w != d_width)
	{
		d_width = w;
		d_valid = false;
	}
	if (h != d_height)
	{
		d_height = h;
		d_valid = false;
	}
	if (scale != d_scale)
	{
		d_scale = scale;
		d_valid = false;
	}
	if (alpha != d_alpha)
	{
		d_alpha = alpha;
		d_valid = false;
	}
	if (back != d_colorids[0])
	{
		d_colorids[0] = back;
		d_valid = false;
	}
}

/*
 * Set the parameters for map drawing.
 *
 * bAbove = only draw surface nodes (i.e. highest non-air nodes)
 * iScan = the number of nodes to scan in each column of the map
 * iSurface = the value of the Y position on the map considered the surface
 * bTracking = stay centered on position
 * iBorder = distance from map border where moved draw is triggered
 *
 * If bAbove is true, then draw will only consider surface nodes. If false, then
 * draw will include consideration of "wall" nodes.
 * 
 * If bTracking is false, then draw only moves the map when the position has
 * entered into a border region.
 */

void gsMapper::setMapType(bool bAbove, u16 iScan, s16 iSurface, bool bTracking, u16 iBorder)
{
	d_above = bAbove;
	d_scan = iScan;
	d_surface = iSurface;
	d_tracking = bTracking;
	d_border = iBorder;
}

/*
 * Perform the actual map draw.
 *
 * position = the focus point of the map
 *
 * This draws the map in increments over a number of calls, accumulating data 
 * with each iteration.
 */

void gsMapper::drawMap(v3s16 position)
{
	// width and height in nodes (these don't really need to be exact)
	s16 nwidth = floor(d_width / d_scale);
	s16 nheight = floor(d_height / d_scale);

	bool hasChanged = false;

	// check if position is outside the center
	if (d_valid)
	{
		if ( (position.X - d_border) < d_origin.X ||
			(position.Z - d_border) < d_origin.Z ||
			(position.X + d_border) > (d_origin.X + nwidth) ||
			(position.Z + d_border) > (d_origin.Z + nheight) ) d_valid = false;
	}

	// recalculate the origin
	if (!d_valid || d_tracking)
	{
		d_origin.X = floor(position.X - (nwidth / 2));
		d_origin.Y = d_surface;
		d_origin.Z = floor(position.Z - (nheight / 2));
		d_valid = true;
		hasChanged = true;
	}

	// rescan next division of the map
	Map &map = d_client->getEnv().getMap();
	v3s16 p;
	s16 x = 0;
	s16 y = 0;
	s16 z = 0;

	while (z < (nheight / 8) && (z + d_scanZ) < nheight)
	{
		p.Z = d_origin.Z + z + d_scanZ;
		x = 0;
		while (x < (nwidth / 8) && (x + d_scanX) < nwidth)
		{
			p.X = d_origin.X + x + d_scanX;
			bool b = true;

			// "above" mode: surface = mid-point for scanning
			if (d_above)
			{
				p.Y = d_surface + (d_scan / 2);
				for (y = 0; y < d_scan; y++)
				{
					if (b)
					{
						MapNode n = map.getNodeNoEx(p);
						p.Y--;
						if (n.param0 != CONTENT_IGNORE && n.param0 != CONTENT_AIR)
						{
							b = false;
							p.Y = d_surface;

							// check to see if this node is different from the map
							std::map<v3s16, u16>::iterator i2 = d_map.find(p);
							if ( i2 == d_map.end() ||
								(i2 != d_map.end() && n.param0 != d_map[p]) )
							{
								hasChanged = true;
								d_map[p] = n.param0;	// change it
							}
						}
					}
				}

			// not "above" = use the radar for mapping
			} else {
				p.Y = position.Y + 1;
				MapNode n = map.getNodeNoEx(p);
				bool w = (n.param0 != CONTENT_IGNORE && n.param0 != CONTENT_AIR);

				int count = 0;
				u16 id = CONTENT_AIR;
				while (b)
				{
					if (w)		// wall = scan up for air
					{
						p.Y++;
						n = map.getNodeNoEx(p);
						if (n.param0 == CONTENT_AIR)
							b = false;
						else
							id = n.param0;

					} else {	// not wall = scan down for non-air
						p.Y--;
						n = map.getNodeNoEx(p);
						if (n.param0 != CONTENT_IGNORE && n.param0 != CONTENT_AIR)
						{
							id = n.param0;
							b = false;
						}
					}
					if (b && count++ >= (d_scan / 8)) 
					{
						b = false;
						id = CONTENT_AIR;
					}
				}

				p.Y = d_surface;		// the data is always flat
				std::map<v3s16, u16>::iterator i5 = d_radar.find(p);
				if ( i5 == d_radar.end() ||
					(i5 != d_radar.end() && id != d_radar[p]) )
				{
					hasChanged = true;
					d_radar[p] = id;	// change it
				}
			}
			x++;
		}
		z++;
	}

	// move the scan block
	d_scanX += (nwidth / 8);
	if (d_scanX >= nwidth)
	{
		d_scanX = 0;
		d_scanZ += (nheight / 8);
		if (d_scanZ >= nheight)	d_scanZ = 0;
	}

	video::IVideoDriver *driver = d_device->getVideoDriver();

	if (hasChanged || !d_hastex)
	{
		// set up the image
		core::dimension2d<u32> dim(nwidth, nheight);
		video::IImage *image = driver->createImage(video::ECF_A8R8G8B8, dim);
		assert(image);

		bool psum = false;
		for (z = 0; z < nheight; z++)
		{
			for (x = 0; x < nwidth; x++)
			{
				p.X = d_origin.X + x;
				p.Y = d_surface;
				p.Z = d_origin.Z + z;

				u16 i = CONTENT_IGNORE;
				if (d_above)
				{
					std::map<v3s16, u16>::iterator i3 = d_map.find(p);
					if (i3 != d_map.end()) i = d_map[p];
				} else {
					std::map<v3s16, u16>::iterator i6 = d_radar.find(p);
					if (i6 != d_radar.end()) i = d_radar[p];
				}

				video::SColor c = getColorFromId(i);
				c.setAlpha(d_alpha);
				image->setPixel(x, nheight - z - 1, c);
				if (i != CONTENT_IGNORE) psum = true;
			}
		}

		// image -> texture
		if (psum && d_cooldown2 == 0)
		{
			if (d_hastex)
			{
				driver->removeTexture(d_texture);
				d_hastex = false;
			}
			std::string f = "gsmapper__" + itos(d_device->getTimer()->getRealTime());
			d_texture = driver->addTexture(f.c_str(), image);
			assert(d_texture);
			d_hastex = true;
			d_cooldown2 = 5;	// don't generate too many textures all at once
		} else {
			d_cooldown2--;
			if (d_cooldown2 < 0) d_cooldown2 = 0;
		}

		image->drop();
	} 

	// draw map texture
	if (d_hastex) {
		driver->draw2DImage( d_texture,
			core::rect<s32>(d_posx, d_posy, d_posx+d_width, d_posy+d_height),
			core::rect<s32>(0, 0, nwidth, nheight),
			0, 0, true );
	}

	// draw local player marker
	if (d_tsrc->isKnownSourceImage("player_marker0.png"))
	{
		v3s16 p = floatToInt(d_player->getPosition(), BS);
		if ( p.X >= d_origin.X && p.X <= (d_origin.X + nwidth) &&
			p.Z >= d_origin.Z && p.Z <= (d_origin.Z + nheight) )
		{
			f32 y = floor(360 - wrapDegrees_0_360(d_player->getYaw()));
			if (y < 0) y = 0;
			int r = floor(y / 90.0);
			int a = floor(fmod(y, 90.0) / 10.0);
			std::string f = "player_marker" + itos(a) + ".png";
			video::ITexture *pmarker = d_tsrc->getTexture(f);
			assert(pmarker);
			v2u32 size = pmarker->getOriginalSize();

			if (r > 0)
			{
				core::dimension2d<u32> dim(size.X, size.Y);
				video::IImage *image1 = driver->createImage(pmarker,
					core::position2d<s32>(0, 0), dim);
				assert(image1);

				// rotate image
				video::IImage *image2 = driver->createImage(video::ECF_A8R8G8B8, dim);
				assert(image2);
				for (u32 y = 0; y < size.Y; y++)
				for (u32 x = 0; x < size.X; x++)
				{
					video::SColor c;
					if (r == 1)
					{
						c = image1->getPixel(y, (size.X - x - 1));
					} else if (r == 2) {
						c = image1->getPixel((size.X - x - 1), (size.Y - y - 1));
					} else {
						c = image1->getPixel((size.Y - y - 1), x);
					}
					image2->setPixel(x, y, c);
				}
				image1->drop();
				if (d_hasptex)
				{
					driver->removeTexture(d_pmarker);
					d_hasptex = false;
				}
				d_pmarker = driver->addTexture("playermarker__temp__", image2);
				assert(d_pmarker);
				pmarker = d_pmarker;
				d_hasptex = true;
				image2->drop();
			}

			s32 sx = d_posx + floor((p.X - d_origin.X) * d_scale) - (size.X / 2);
			s32 sy = d_posy + floor((nheight - (p.Z - d_origin.Z)) * d_scale) - (size.Y / 2);
			driver->draw2DImage( pmarker,
				core::rect<s32>(sx, sy, sx+size.X, sy+size.Y),
				core::rect<s32>(0, 0, size.X, size.Y),
				0, 0, true );
		}
	}
}
