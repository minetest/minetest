// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include <irrlicht.h>
#include "ITextSceneNode.h"
#include "IBillboardTextSceneNode.h"
#include "IGUIFont.h"
#include "IGUIFontBitmap.h"
#include "ISceneCollisionManager.h"
#include "SMesh.h"
#include <string>

namespace irr
{
namespace scene
{
	class CTextSpriteSceneNode : public IBillboardTextSceneNode
	{
	public:

		CTextSpriteSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
			gui::IGUIFont* font,const wchar_t* text,
			const core::vector3df& position, const core::dimension2d<f32>& size,
			const video::SColor colorTop, const video::SColor colorBottom,
			const bool background, const video::SColor & backgroundColor, 
			const video::SColor & borderColor, const f32 border, 
			const f32 xPadding, const f32 yPadding,
			const f32 xOffset, const f32 yOffset, 
			const f32 spacing, const f32 baseOffset);

		//! destructor
		virtual ~CTextSpriteSceneNode();

		//! sets the vertex positions etc
		virtual void OnAnimate(u32 timeMs);

		//! registers the node into the transparent pass
		virtual void OnRegisterSceneNode();

		//! renders the background.
		virtual void renderBackground();
		
		//! renders the node.
		virtual void render();

		//! returns the axis aligned bounding box of this node
		virtual const core::aabbox3d<f32>& getBoundingBox() const;

		//! sets the text string
		virtual void setText(const wchar_t* text);

		//! sets the color of the text
		virtual void setTextColor(video::SColor color);

		//! sets the size of the billboard
		virtual void setSize(const core::dimension2d<f32>& size);
		
		//! resize the billboard
		virtual void resize();

		//! gets the size of the billboard
		virtual const core::dimension2d<f32>& getSize() const;

		virtual video::SMaterial& getMaterial(u32 i);

		//! returns amount of materials used by this scene node.
		virtual u32 getMaterialCount() const;

		//! Returns type of the scene node
		virtual ESCENE_NODE_TYPE getType() const { return ESNT_TEXT; }

		//! Set the color of all vertices of the billboard
		//! \param overallColor: the color to set
		virtual void setColor(const video::SColor & overallColor);

		//! Set the color of the top and bottom vertices of the billboard
		//! \param topColor: the color to set the top vertices
		//! \param bottomColor: the color to set the bottom vertices
		virtual void setColor(const video::SColor & topColor, const video::SColor & bottomColor);

		//! Gets the color of the top and bottom vertices of the billboard
		//! \param topColor: stores the color of the top vertices
		//! \param bottomColor: stores the color of the bottom vertices
		virtual void getColor(video::SColor & topColor, video::SColor & bottomColor) const;

		//! Sets the size
		virtual void setSize(f32 height, f32 bottomEdgeWidth, f32 topEdgeWidth)
		{
			setSize(core::dimension2df(bottomEdgeWidth, height));
		}

		//! Gets the size
		virtual void getSize(f32& height, f32& bottomEdgeWidth, f32& topEdgeWidth) const
		{
			height = Size.Height;
			bottomEdgeWidth = Size.Width;
			topEdgeWidth = Size.Width;
		}

		
		//! Adds a text scene node, which uses billboards
		static CTextSpriteSceneNode* addBillboardTextSceneNode(gui::IGUIFont* font, const wchar_t* text,
			gui::IGUIEnvironment* environment, ISceneManager* smgr, ISceneNode* parent = 0,
			const core::dimension2d<f32>& size = core::dimension2d<f32>(10.0f, 10.0f),
			const core::vector3df& position = core::vector3df(0,0,0), s32 id=-1,
			const video::SColor colorTop = 0xFFFFFFFF, const video::SColor colorBottom = 0xFFFFFFFF,
			const bool background = false, const video::SColor & backgroundColor = 0xFFFFFFFF, 
			const video::SColor & borderColor = 0xFFFFFFFF, const f32 border = 0.0f,
			const f32 xPadding = 0.0f, const f32 yPadding = 0.0f,
			const f32 xOffset = 0.0f, const f32 yOffset = 0.0f, 
			const f32 spacing = 1.0f, const f32 baseOffset = 0.0f);
	private:
		
		//! Parses an hexadecimal color
		void parseColor(const std::string& color_string,
						video::SColor& color, f32& scale);
		
		core::stringw OldText;
		core::stringw Text;
		f32 LineCount;
		gui::IGUIFontBitmap* Font;
		core::dimension2d<f32> Size;
		core::aabbox3d<f32> BBox;
		video::SMaterial Material;

		video::SColor TopColor;
		video::SColor BottomColor;
		
		bool Background;
		video::SColor BackgroundColor;
		video::SColor BorderColor;
		f32 Border;
		f32 XPadding;
		f32 YPadding;
		f32 XOffset;
		f32 YOffset;
		f32 Spacing;
		f32 BaseOffset;
		f32 Width;
		f32 Height;
		
		struct SSymbolInfo
		{
			u32 bufNo;
			f32 LineBreaks;
			f32 Scale;
			f32 XPosition;
			f32 YPosition;
			f32 Kerning;
			f32 Width;
			f32 Height;
			f32 LineHeight;
			f32 BaseHeight;
			f32 LineBaseHeight;
			u32 firstInd;
			u32 firstVert;
			video::SColor TopColor;
			video::SColor BottomColor;
		};

		core::array < SSymbolInfo > Symbol;

		SMesh *Mesh;
	};

} // end namespace scene
} // end namespace irr
