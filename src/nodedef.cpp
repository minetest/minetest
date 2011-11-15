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

#include "nodedef.h"

#include "main.h" // For g_settings
#include "nodemetadata.h"
#ifndef SERVER
#include "tile.h"
#endif
#include "log.h"

ContentFeatures::~ContentFeatures()
{
	delete initial_metadata;
#ifndef SERVER
	delete special_material;
	delete special_atlas;
#endif
}

#if 0
void ContentFeatures::setTexture(ITextureSource *tsrc,
		u16 i, std::string name, u8 alpha)
{
	used_texturenames.insert(name);
	
	if(tsrc)
	{
		tiles[i].texture = tsrc->getTexture(name);
	}
	
	if(alpha != 255)
	{
		tiles[i].alpha = alpha;
		tiles[i].material_type = MATERIAL_ALPHA_VERTEX;
	}

	if(inventory_texture == NULL)
		setInventoryTexture(name, tsrc);
}

void ContentFeatures::setInventoryTexture(std::string imgname,
		ITextureSource *tsrc)
{
	if(tsrc == NULL)
		return;
	
	imgname += "^[forcesingle";
	
	inventory_texture = tsrc->getTextureRaw(imgname);
}

void ContentFeatures::setInventoryTextureCube(std::string top,
		std::string left, std::string right, ITextureSource *tsrc)
{
	if(tsrc == NULL)
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
	inventory_texture = tsrc->getTextureRaw(imgname_full);
}
#endif

void ContentFeatures::setTexture(u16 i, std::string name)
{
	used_texturenames.insert(name);
	tname_tiles[i] = name;
	if(tname_inventory == "")
		tname_inventory = name;
}

void ContentFeatures::setInventoryTexture(std::string imgname)
{
	tname_inventory = imgname + "^[forcesingle";
}

void ContentFeatures::setInventoryTextureCube(std::string top,
		std::string left, std::string right)
{
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
	tname_inventory = imgname_full;
}

class CNodeDefManager: public IWritableNodeDefManager
{
public:
	CNodeDefManager()
	{
		for(u16 i=0; i<=MAX_CONTENT; i++)
		{
			ContentFeatures *f = &m_content_features[i];
			// Reset to defaults
			f->reset();
			if(i == CONTENT_IGNORE || i == CONTENT_AIR)
				continue;
			f->setAllTextures("unknown_block.png");
			f->dug_item = std::string("MaterialItem2 ")+itos(i)+" 1";
		}
		// Make CONTENT_IGNORE to not block the view when occlusion culling
		m_content_features[CONTENT_IGNORE].solidness = 0;
	}
	virtual ~CNodeDefManager()
	{
	}
	virtual IWritableNodeDefManager* clone()
	{
		CNodeDefManager *mgr = new CNodeDefManager();
		for(u16 i=0; i<=MAX_CONTENT; i++)
		{
			mgr->set(i, get(i));
		}
		return mgr;
	}
	virtual const ContentFeatures& get(content_t c) const
	{
		assert(c <= MAX_CONTENT);
		return m_content_features[c];
	}
	virtual const ContentFeatures& get(const MapNode &n) const
	{
		return get(n.getContent());
	}
	// Writable
	virtual void set(content_t c, const ContentFeatures &def)
	{
		infostream<<"registerNode: registering content \""<<c<<"\""<<std::endl;
		assert(c <= MAX_CONTENT);
		m_content_features[c] = def;
	}
	virtual ContentFeatures* getModifiable(content_t c)
	{
		assert(c <= MAX_CONTENT);
		return &m_content_features[c];
	}
	virtual void updateTextures(ITextureSource *tsrc)
	{
#ifndef SERVER
		infostream<<"CNodeDefManager::updateTextures(): Updating "
				<<"textures in node definitions"<<std::endl;
		for(u16 i=0; i<=MAX_CONTENT; i++)
		{
			infostream<<"Updating content "<<i<<std::endl;
			ContentFeatures *f = &m_content_features[i];
			// Inventory texture
			if(f->tname_inventory != "")
				f->inventory_texture = tsrc->getTextureRaw(f->tname_inventory);
			else
				f->inventory_texture = NULL;
			// Tile textures
			for(u16 j=0; j<6; j++){
				if(f->tname_tiles[j] == "")
					continue;
				f->tiles[j].texture = tsrc->getTexture(f->tname_tiles[j]);
				f->tiles[j].alpha = f->alpha;
				if(f->alpha == 255)
					f->tiles[j].material_type = MATERIAL_ALPHA_SIMPLE;
				else
					f->tiles[j].material_type = MATERIAL_ALPHA_VERTEX;
				if(f->backface_culling)
					f->tiles[j].material_flags |= MATERIAL_FLAG_BACKFACE_CULLING;
				else
					f->tiles[j].material_flags &= ~MATERIAL_FLAG_BACKFACE_CULLING;
			}
			// Special texture
			if(f->tname_special != ""){
				if(!f->special_atlas)
					f->special_atlas = new AtlasPointer(
							tsrc->getTexture(f->tname_special));
				else
					*(f->special_atlas) =
							tsrc->getTexture(f->tname_special);
				// Special material textures
				if(f->special_material)
					f->special_material->setTexture(0, f->special_atlas->atlas);
				if(f->special_material2)
					f->special_material2->setTexture(0, f->special_atlas->atlas);
			}
		}
#endif
	}
private:
	ContentFeatures m_content_features[MAX_CONTENT+1];
};

IWritableNodeDefManager* createNodeDefManager()
{
	return new CNodeDefManager();
}

