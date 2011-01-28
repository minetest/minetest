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

#ifndef IIRRLICHTWRAPPER_HEADER
#define IIRRLICHTWRAPPER_HEADER

#include "common_irrlicht.h"

/*
	IrrlichtWrapper prototype.

	Server supplies this as a dummy wrapper.
*/

class IIrrlichtWrapper
{
public:
	IIrrlichtWrapper()
	{
	}
	virtual ~IIrrlichtWrapper()
	{
	}
	
	virtual u32 getTime()
	{
		return 0;
	}
	
	virtual textureid_t getTextureId(const std::string &name){ return 0; }
	virtual std::string getTextureName(textureid_t id){ return ""; }
	virtual video::ITexture* getTexture(const std::string &name){ return NULL; }
	virtual video::ITexture* getTexture(const TextureSpec &spec){ return NULL; }
	
private:
};

#endif

