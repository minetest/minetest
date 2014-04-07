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
#include "util/numeric.h"
#include "util/string.h"
#include <math.h>

gsMapper::gsMapper(IrrlichtDevice *device, Client *client):
	d_device(device),
	d_client(client)
{
	// note: max = 255, alpha values are ignored
	d_colorids.clear();
	d_nodeids.clear();

	d_colorids.push_back(video::SColor(0,0,0,0));		// 0 = background
	d_nodeids["air"] = 0;
	d_colorids.push_back(video::SColor(0,99,99,99));	// 1 = mystery node
	d_nodeids["unknown"] = 1;
	d_colorids.push_back(video::SColor(0,32,32,192));	// 2+ = standard nodes
	d_nodeids["default:water_source"] = 2;
	d_colorids.push_back(video::SColor(0,64,64,224));
	d_nodeids["default:water_flowing"] = 3;
	d_colorids.push_back(video::SColor(0,32,128,16));
	d_nodeids["default:dirt_with_grass"] = 4;
	d_colorids.push_back(video::SColor(0,240,240,240));
	d_nodeids["default:dirt_with_snow"] = 5;
	d_colorids.push_back(video::SColor(0,128,128,96));
	d_nodeids["default:dirt"] = 6;
	d_colorids.push_back(video::SColor(0,64,64,64));
	d_nodeids["default:stone"] = 7;
	d_colorids.push_back(video::SColor(0,144,144,0));
	d_nodeids["default:desert_stone"] = 8;
	d_colorids.push_back(video::SColor(0,192,192,160));
	d_nodeids["default:sand"] = 9;
	d_colorids.push_back(video::SColor(0,160,160,96));
	d_nodeids["default:desert_sand"] = 10;
	d_colorids.push_back(video::SColor(0,32,64,32));
	d_nodeids["default:tree"] = 11;
	d_colorids.push_back(video::SColor(0,32,96,32));
	d_nodeids["default:cactus"] = 12;
	d_colorids.push_back(video::SColor(0,64,96,0));
	d_nodeids["default:wood"] = 13;
	d_colorids.push_back(video::SColor(0,16,64,16));
	d_nodeids["default:jungletree"] = 14;
	d_colorids.push_back(video::SColor(0,32,64,0));
	d_nodeids["default:junglewood"] = 15;

	d_valid = false;
	d_hastex = false;
	d_hasptex = false;
	d_cooldown = 0;
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

	if (d_cooldown++ < 200) return;
	d_cooldown = 200;
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
	// I have no idea why this might be necessary, but... whatever
	if (d_cooldown < 200) return;
	d_cooldown = 200;

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
							const ContentFeatures &f = d_client->getNodeDefManager()->get(n);
							b = false;
							std::string name = f.name;
							std::map<std::string, u8>::iterator i1 = d_nodeids.find(name);
							p.Y = d_surface;
	
							u8 id = 1;	// unknown
							if (i1 != d_nodeids.end()) id = d_nodeids[name];
	
							// check to see if this node is different from the map
							std::map<v3s16, u8>::iterator i2 = d_map.find(p);
							if ( i2 == d_map.end() ||
								(i2 != d_map.end() && id != d_map[p]) )
							{
								hasChanged = true;
								d_map[p] = id;	// change it
							}
						}
					}
				}

			// not "above" = use the radar for mapping
			} else {
				p.Y = position.Y;
				s16 y2 = position.Y;
				MapNode n = map.getNodeNoEx(p);
				bool w = (n.param0 != CONTENT_IGNORE && n.param0 != CONTENT_AIR);
				int count = 0;
				u8 id = 0;
				while (b && id == 0)
				{
					if (w)		// wall = scan up for air
					{
						p.Y++;
						n = map.getNodeNoEx(p);
						if (n.param0 == CONTENT_AIR) id = 1;

					} else {	// not wall = scan down for non-air
						p.Y--;
						n = map.getNodeNoEx(p);
						if (n.param0 != CONTENT_IGNORE && n.param0 != CONTENT_AIR)
						{
							y2 = p.Y;
							id = 1;
						}
					}
					if (id == 0 && n.param0 != CONTENT_IGNORE) y2 = p.Y;
					if (count++ >= (d_scan / 8)) b = false;
				}

				p.Y = y2;
				n = map.getNodeNoEx(p);
				u8 id2 = 1;
				if (n.param0 != CONTENT_IGNORE && n.param0 != CONTENT_AIR)
				{
					const ContentFeatures &f = d_client->getNodeDefManager()->get(n);
					std::string name = f.name;
					std::map<std::string, u8>::iterator i4 = d_nodeids.find(name);
					if (i4 != d_nodeids.end()) id2 = d_nodeids[name];
				}

				// check for under water
				if (id == 0 && w) {
					if (id2 == 2) id = 2;
				}

				if (id == 1) id = id2;	// found something

				p.Y = d_surface;		// the data is always flat
				std::map<v3s16, u8>::iterator i5 = d_radar.find(p);
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

	video::IVideoDriver* driver = d_device->getVideoDriver();

	if (hasChanged || !d_hastex)
	{
		// set up the image
		core::dimension2d<u32> dim(nwidth, nheight);
		video::IImage *image = driver->createImage(video::ECF_A8R8G8B8, dim);
		assert(image);

		for (z = 0; z < nheight; z++)
		{
			for (x = 0; x < nwidth; x++)
			{
				p.X = d_origin.X + x;
				p.Y = d_surface;
				p.Z = d_origin.Z + z;

				u8 i = 0;
				if (d_above)
				{
					std::map<v3s16, u8>::iterator i3 = d_map.find(p);
					if (i3 != d_map.end()) i = d_map[p];
				} else {
					std::map<v3s16, u8>::iterator i6 = d_radar.find(p);
					if (i6 != d_radar.end()) i = d_radar[p];
				}

				video::SColor c = d_colorids[i];
				c.setAlpha(d_alpha);
				image->setPixel(x, nheight - z - 1, c);
			}
		}

		// image -> texture
		if (d_hastex)
		{
			driver->removeTexture(d_texture);
			d_hastex = false;
		}
		std::string f = "gsmapper__" + itos(d_device->getTimer()->getRealTime());
		d_texture = driver->addTexture(f.c_str(), image);
		assert(d_texture);
		d_hastex = true;
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
