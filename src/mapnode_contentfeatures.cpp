/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "mapnode_contentfeatures.h"

#include "main.h" // For g_settings and g_texturesource
#include "nodemetadata.h"

struct ContentFeatures g_content_features[MAX_CONTENT+1];

/*
	Initialize content feature table.

	Must be called before accessing the table.
*/
void init_contentfeatures()
{
#ifndef SERVER
	/*
		Set initial material type to same in all tiles, so that the
		same material can be used in more stuff.
		This is set according to the leaves because they are the only
		differing material to which all materials can be changed to
		get this optimization.
	*/
	u8 initial_material_type = MATERIAL_ALPHA_SIMPLE;
	/*if(new_style_leaves)
		initial_material_type = MATERIAL_ALPHA_SIMPLE;
	else
		initial_material_type = MATERIAL_ALPHA_NONE;*/
	for(u16 i=0; i<MAX_CONTENT+1; i++)
	{
		ContentFeatures *f = &g_content_features[i];
		// Re-initialize
		f->reset();

		for(u16 j=0; j<6; j++)
			f->tiles[j].material_type = initial_material_type;
	}
#endif

	/*
		Initially set every block to be shown as an unknown block.
		Don't touch CONTENT_IGNORE or CONTENT_AIR.
	*/
	for(u16 i=0; i<MAX_CONTENT+1; i++)
	{
		if(i == CONTENT_IGNORE || i == CONTENT_AIR)
			continue;
		ContentFeatures *f = &g_content_features[i];
		f->setAllTextures("unknown_block.png");
		f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
	}

	// Make CONTENT_IGNORE to not block the view when occlusion culling
	content_features(CONTENT_IGNORE).solidness = 0;

}

ContentFeatures::~ContentFeatures()
{
	delete initial_metadata;
#ifndef SERVER
	delete special_material;
	delete special_atlas;
#endif
}

#ifndef SERVER
void ContentFeatures::setTexture(u16 i, std::string name, u8 alpha)
{
	used_texturenames[name] = true;
	
	if(g_texturesource)
	{
		tiles[i].texture = g_texturesource->getTexture(name);
	}
	
	if(alpha != 255)
	{
		tiles[i].alpha = alpha;
		tiles[i].material_type = MATERIAL_ALPHA_VERTEX;
	}

	if(inventory_texture == NULL)
		setInventoryTexture(name);
}

void ContentFeatures::setInventoryTexture(std::string imgname)
{
	if(g_texturesource == NULL)
		return;
	
	imgname += "^[forcesingle";
	
	inventory_texture = g_texturesource->getTextureRaw(imgname);
}

void ContentFeatures::setInventoryTextureCube(std::string top,
		std::string left, std::string right)
{
	if(g_texturesource == NULL)
		return;
	
	str_replace_char(top, '^', '&');
	str_replace_char(left, '^', '&');
	str_replace_char(right, '^', '&');

	std::string imgname_full;
	imgname_full += "[inventorycube{";
	imgname_full += top;
	imgname_full += "{";
	imgname_full += left;
	imgname_full += "{";
	imgname_full += right;
	inventory_texture = g_texturesource->getTextureRaw(imgname_full);
}
#endif

ContentFeatures & content_features(content_t i)
{
	return g_content_features[i];
}
ContentFeatures & content_features(MapNode &n)
{
	return content_features(n.getContent());
}


