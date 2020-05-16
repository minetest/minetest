// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_GUI_SPRITE_BANK_H_INCLUDED__
#define __I_GUI_SPRITE_BANK_H_INCLUDED__

#include "IReferenceCounted.h"
#include "irrArray.h"
#include "SColor.h"
#include "rect.h"

namespace irr
{

namespace video
{
	class ITexture;
} // end namespace video

namespace gui
{

//! A single sprite frame.
struct SGUISpriteFrame
{
	u32 textureNumber;
	u32 rectNumber;
};

//! A sprite composed of several frames.
struct SGUISprite
{
	SGUISprite() : Frames(), frameTime(0) {}

	core::array<SGUISpriteFrame> Frames;
	u32 frameTime;
};


//! Sprite bank interface.
/** See http://irrlicht.sourceforge.net/phpBB2/viewtopic.php?t=25742&highlight=spritebank
* for more information how to use the spritebank.
*/
class IGUISpriteBank : public virtual IReferenceCounted
{
public:

	//! Returns the list of rectangles held by the sprite bank
	virtual core::array< core::rect<s32> >& getPositions() = 0;

	//! Returns the array of animated sprites within the sprite bank
	virtual core::array< SGUISprite >& getSprites() = 0;

	//! Returns the number of textures held by the sprite bank
	virtual u32 getTextureCount() const = 0;

	//! Gets the texture with the specified index
	virtual video::ITexture* getTexture(u32 index) const = 0;

	//! Adds a texture to the sprite bank
	virtual void addTexture(video::ITexture* texture) = 0;

	//! Changes one of the textures in the sprite bank
	virtual void setTexture(u32 index, video::ITexture* texture) = 0;

	//! Add the texture and use it for a single non-animated sprite.
	//! The texture and the corresponding rectangle and sprite will all be added to the end of each array.
	//! returns the index of the sprite or -1 on failure
	virtual s32 addTextureAsSprite(video::ITexture* texture) = 0;

	//! clears sprites, rectangles and textures
	virtual void clear() = 0;

	//! Draws a sprite in 2d with position and color
	virtual void draw2DSprite(u32 index, const core::position2di& pos,
			const core::rect<s32>* clip=0,
			const video::SColor& color= video::SColor(255,255,255,255),
			u32 starttime=0, u32 currenttime=0,
			bool loop=true, bool center=false) = 0;

	//! Draws a sprite batch in 2d using an array of positions and a color
	virtual void draw2DSpriteBatch(const core::array<u32>& indices, const core::array<core::position2di>& pos,
			const core::rect<s32>* clip=0,
			const video::SColor& color= video::SColor(255,255,255,255),
			u32 starttime=0, u32 currenttime=0,
			bool loop=true, bool center=false) = 0;
};


} // end namespace gui
} // end namespace irr

#endif // __I_GUI_SPRITE_BANK_H_INCLUDED__

