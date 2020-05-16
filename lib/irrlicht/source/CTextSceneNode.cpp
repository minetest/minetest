// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CTextSceneNode.h"
#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "ICameraSceneNode.h"
#include "IGUISpriteBank.h"
#include "SMeshBuffer.h"
#include "os.h"


namespace irr
{
namespace scene
{


//! constructor
CTextSceneNode::CTextSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
			gui::IGUIFont* font, scene::ISceneCollisionManager* coll,
			const core::vector3df& position, const wchar_t* text,
			video::SColor color)
	: ITextSceneNode(parent, mgr, id, position), Text(text), Color(color),
		Font(font), Coll(coll)

{
	#ifdef _DEBUG
	setDebugName("CTextSceneNode");
	#endif

	if (Font)
		Font->grab();

	setAutomaticCulling(scene::EAC_OFF);
}

//! destructor
CTextSceneNode::~CTextSceneNode()
{
	if (Font)
		Font->drop();
}

void CTextSceneNode::OnRegisterSceneNode()
{
	if (IsVisible)
		SceneManager->registerNodeForRendering(this, ESNRP_TRANSPARENT);

	ISceneNode::OnRegisterSceneNode();
}

//! renders the node.
void CTextSceneNode::render()
{
	if (!Font || !Coll)
		return;

	core::position2d<s32> pos = Coll->getScreenCoordinatesFrom3DPosition(getAbsolutePosition(),
		SceneManager->getActiveCamera());

	core::rect<s32> r(pos, core::dimension2d<s32>(1,1));
	Font->draw(Text.c_str(), r, Color, true, true);
}


//! returns the axis aligned bounding box of this node
const core::aabbox3d<f32>& CTextSceneNode::getBoundingBox() const
{
	return Box;
}

//! sets the text string
void CTextSceneNode::setText(const wchar_t* text)
{
	Text = text;
}


//! sets the color of the text
void CTextSceneNode::setTextColor(video::SColor color)
{
	Color = color;
}


//!--------------------------------- CBillboardTextSceneNode ----------------------------------------------


//! constructor
CBillboardTextSceneNode::CBillboardTextSceneNode(ISceneNode* parent, ISceneManager* mgr, s32 id,
	gui::IGUIFont* font,const wchar_t* text,
	const core::vector3df& position, const core::dimension2d<f32>& size,
	video::SColor colorTop,video::SColor shade_bottom )
: IBillboardTextSceneNode(parent, mgr, id, position),
	Font(0), ColorTop(colorTop), ColorBottom(shade_bottom), Mesh(0)
{
	#ifdef _DEBUG
	setDebugName("CBillboardTextSceneNode");
	#endif

	Material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	Material.MaterialTypeParam = 1.f / 255.f;
	Material.BackfaceCulling = false;
	Material.Lighting = false;
	Material.ZBuffer = video::ECFN_LESSEQUAL;
	Material.ZWriteEnable = false;

	if (font)
	{
		// doesn't support other font types
		if (font->getType() == gui::EGFT_BITMAP)
		{
			Font = (gui::IGUIFontBitmap*)font;
			Font->grab();

			// mesh with one buffer per texture
			Mesh = new SMesh();
			for (u32 i=0; i<Font->getSpriteBank()->getTextureCount(); ++i)
			{
				SMeshBuffer *mb = new SMeshBuffer();
				mb->Material = Material;
				mb->Material.setTexture(0, Font->getSpriteBank()->getTexture(i));
				Mesh->addMeshBuffer(mb);
				mb->drop();
			}
		}
		else
		{
			os::Printer::log("Sorry, CBillboardTextSceneNode does not support this font type", ELL_INFORMATION);
		}
	}

	setText(text);
	setSize(size);

	setAutomaticCulling ( scene::EAC_BOX );
}



CBillboardTextSceneNode::~CBillboardTextSceneNode()
{
	if (Font)
		Font->drop();

	if (Mesh)
		Mesh->drop();

}


//! sets the text string
void CBillboardTextSceneNode::setText(const wchar_t* text)
{
	if ( !Mesh )
		return;

	Text = text;

	Symbol.clear();

	// clear mesh
	for (u32 j=0; j < Mesh->getMeshBufferCount(); ++j)
	{
		((SMeshBuffer*)Mesh->getMeshBuffer(j))->Indices.clear();
		((SMeshBuffer*)Mesh->getMeshBuffer(j))->Vertices.clear();
	}

	if (!Font)
		return;

	const core::array< core::rect<s32> > &sourceRects = Font->getSpriteBank()->getPositions();
	const core::array< gui::SGUISprite > &sprites = Font->getSpriteBank()->getSprites();

	f32 dim[2];
	f32 tex[4];

	u32 i;
	for ( i = 0; i != Text.size (); ++i )
	{
		SSymbolInfo info;

		u32 spriteno = Font->getSpriteNoFromChar( &text[i] );
		u32 rectno = sprites[spriteno].Frames[0].rectNumber;
		u32 texno = sprites[spriteno].Frames[0].textureNumber;

		dim[0] = core::reciprocal ( (f32) Font->getSpriteBank()->getTexture(texno)->getSize().Width );
		dim[1] = core::reciprocal ( (f32) Font->getSpriteBank()->getTexture(texno)->getSize().Height );

		const core::rect<s32>& s = sourceRects[rectno];

		// add space for letter to buffer
		SMeshBuffer* buf = (SMeshBuffer*)Mesh->getMeshBuffer(texno);
		u32 firstInd = buf->Indices.size();
		u32 firstVert = buf->Vertices.size();
		buf->Indices.set_used(firstInd + 6);
		buf->Vertices.set_used(firstVert + 4);

		tex[0] = (s.LowerRightCorner.X * dim[0]) + 0.5f*dim[0]; // half pixel
		tex[1] = (s.LowerRightCorner.Y * dim[1]) + 0.5f*dim[1];
		tex[2] = (s.UpperLeftCorner.Y  * dim[1]) - 0.5f*dim[1];
		tex[3] = (s.UpperLeftCorner.X  * dim[0]) - 0.5f*dim[0];

		buf->Vertices[firstVert+0].TCoords.set(tex[0], tex[1]);
		buf->Vertices[firstVert+1].TCoords.set(tex[0], tex[2]);
		buf->Vertices[firstVert+2].TCoords.set(tex[3], tex[2]);
		buf->Vertices[firstVert+3].TCoords.set(tex[3], tex[1]);

		buf->Vertices[firstVert+0].Color = ColorBottom;
		buf->Vertices[firstVert+3].Color = ColorBottom;
		buf->Vertices[firstVert+1].Color = ColorTop;
		buf->Vertices[firstVert+2].Color = ColorTop;

		buf->Indices[firstInd+0] = (u16)firstVert+0;
		buf->Indices[firstInd+1] = (u16)firstVert+2;
		buf->Indices[firstInd+2] = (u16)firstVert+1;
		buf->Indices[firstInd+3] = (u16)firstVert+0;
		buf->Indices[firstInd+4] = (u16)firstVert+3;
		buf->Indices[firstInd+5] = (u16)firstVert+2;

		wchar_t *tp = 0;
		if (i>0)
			tp = &Text[i-1];

		info.Width = (f32)s.getWidth();
		info.bufNo = texno;
		info.Kerning = (f32)Font->getKerningWidth(&Text[i], tp);
		info.firstInd = firstInd;
		info.firstVert = firstVert;

		Symbol.push_back(info);
	}
}


//! pre render event
void CBillboardTextSceneNode::OnAnimate(u32 timeMs)
{
	ISceneNode::OnAnimate(timeMs);
	
	if (!IsVisible || !Font || !Mesh)
		return;

	ICameraSceneNode* camera = SceneManager->getActiveCamera();
	if (!camera)
		return;

	// get text width
	f32 textLength = 0.f;
	u32 i;
	for(i=0; i!=Symbol.size(); ++i)
	{
		SSymbolInfo &info = Symbol[i];
		textLength += info.Kerning + info.Width;
	}
	if (textLength<0.0f)
		textLength=1.0f;

	//const core::matrix4 &m = camera->getViewFrustum()->Matrices[ video::ETS_VIEW ];

	// make billboard look to camera
	core::vector3df pos = getAbsolutePosition();

	core::vector3df campos = camera->getAbsolutePosition();
	core::vector3df target = camera->getTarget();
	core::vector3df up = camera->getUpVector();
	core::vector3df view = target - campos;
	view.normalize();

	core::vector3df horizontal = up.crossProduct(view);
	if ( horizontal.getLength() == 0 )
	{
		horizontal.set(up.Y,up.X,up.Z);
	}

	horizontal.normalize();
	core::vector3df space = horizontal;

	horizontal *= 0.5f * Size.Width;

	core::vector3df vertical = horizontal.crossProduct(view);
	vertical.normalize();
	vertical *= 0.5f * Size.Height;

	view *= -1.0f;

	// center text
	pos += space * (Size.Width * -0.5f);

	for ( i = 0; i!= Symbol.size(); ++i )
	{
		SSymbolInfo &info = Symbol[i];
		f32 infw = info.Width / textLength;
		f32 infk = info.Kerning / textLength;
		f32 w = (Size.Width * infw * 0.5f);
		pos += space * w;

		SMeshBuffer* buf = (SMeshBuffer*)Mesh->getMeshBuffer(info.bufNo);

		buf->Vertices[info.firstVert+0].Normal = view;
		buf->Vertices[info.firstVert+1].Normal = view;
		buf->Vertices[info.firstVert+2].Normal = view;
		buf->Vertices[info.firstVert+3].Normal = view;

		buf->Vertices[info.firstVert+0].Pos = pos + (space * w) + vertical;
		buf->Vertices[info.firstVert+1].Pos = pos + (space * w) - vertical;
		buf->Vertices[info.firstVert+2].Pos = pos - (space * w) - vertical;
		buf->Vertices[info.firstVert+3].Pos = pos - (space * w) + vertical;

		pos += space * (Size.Width*infk + w);
	}

	// make bounding box

	for (i=0; i< Mesh->getMeshBufferCount() ; ++i)
		Mesh->getMeshBuffer(i)->recalculateBoundingBox();
	Mesh->recalculateBoundingBox();

	BBox = Mesh->getBoundingBox();
	core::matrix4 mat( getAbsoluteTransformation(), core::matrix4::EM4CONST_INVERSE );
	mat.transformBoxEx(BBox);
}

void CBillboardTextSceneNode::OnRegisterSceneNode()
{
	SceneManager->registerNodeForRendering(this, ESNRP_TRANSPARENT);
	ISceneNode::OnRegisterSceneNode();
}


//! render
void CBillboardTextSceneNode::render()
{
	if ( !Mesh )
		return;

	video::IVideoDriver* driver = SceneManager->getVideoDriver();

	// draw
	core::matrix4 mat;
	driver->setTransform(video::ETS_WORLD, mat);

	for (u32 i = 0; i < Mesh->getMeshBufferCount(); ++i)
	{
		driver->setMaterial(Mesh->getMeshBuffer(i)->getMaterial());
		driver->drawMeshBuffer(Mesh->getMeshBuffer(i));
	}

	if ( DebugDataVisible & scene::EDS_BBOX )
	{
		driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
		video::SMaterial m;
		m.Lighting = false;
		driver->setMaterial(m);
		driver->draw3DBox(BBox, video::SColor(0,208,195,152));
	}
}


//! returns the axis aligned bounding box of this node
const core::aabbox3d<f32>& CBillboardTextSceneNode::getBoundingBox() const
{
	return BBox;
}


//! sets the size of the billboard
void CBillboardTextSceneNode::setSize(const core::dimension2d<f32>& size)
{
	Size = size;

	if (Size.Width == 0.0f)
		Size.Width = 1.0f;

	if (Size.Height == 0.0f )
		Size.Height = 1.0f;

	//f32 avg = (size.Width + size.Height)/6;
	//BBox.MinEdge.set(-avg,-avg,-avg);
	//BBox.MaxEdge.set(avg,avg,avg);
}


video::SMaterial& CBillboardTextSceneNode::getMaterial(u32 i)
{
	if (Mesh && Mesh->getMeshBufferCount() > i )
		return Mesh->getMeshBuffer(i)->getMaterial();
	else
		return Material;
}


//! returns amount of materials used by this scene node.
u32 CBillboardTextSceneNode::getMaterialCount() const
{
	if (Mesh)
		return Mesh->getMeshBufferCount();
	else
		return 0;
}


//! gets the size of the billboard
const core::dimension2d<f32>& CBillboardTextSceneNode::getSize() const
{
	return Size;
}


//! sets the color of the text
void CBillboardTextSceneNode::setTextColor(video::SColor color)
{
	Color = color;
}

//! Set the color of all vertices of the billboard
//! \param overallColor: the color to set
void CBillboardTextSceneNode::setColor(const video::SColor & overallColor)
{
	if ( !Mesh )
		return;

	for ( u32 i = 0; i != Text.size (); ++i )
	{
		const SSymbolInfo &info = Symbol[i];
		SMeshBuffer* buf = (SMeshBuffer*)Mesh->getMeshBuffer(info.bufNo);
		buf->Vertices[info.firstVert+0].Color = overallColor;
		buf->Vertices[info.firstVert+1].Color = overallColor;
		buf->Vertices[info.firstVert+2].Color = overallColor;
		buf->Vertices[info.firstVert+3].Color = overallColor;
	}
}


//! Set the color of the top and bottom vertices of the billboard
//! \param topColor: the color to set the top vertices
//! \param bottomColor: the color to set the bottom vertices
void CBillboardTextSceneNode::setColor(const video::SColor & topColor, const video::SColor & bottomColor)
{
	if ( !Mesh )
		return;

	ColorBottom = bottomColor;
	ColorTop = topColor;
	for ( u32 i = 0; i != Text.size (); ++i )
	{
		const SSymbolInfo &info = Symbol[i];
		SMeshBuffer* buf = (SMeshBuffer*)Mesh->getMeshBuffer(info.bufNo);
		buf->Vertices[info.firstVert+0].Color = ColorBottom;
		buf->Vertices[info.firstVert+3].Color = ColorBottom;
		buf->Vertices[info.firstVert+1].Color = ColorTop;
		buf->Vertices[info.firstVert+2].Color = ColorTop;
	}
}


//! Gets the color of the top and bottom vertices of the billboard
//! \param topColor: stores the color of the top vertices
//! \param bottomColor: stores the color of the bottom vertices
void CBillboardTextSceneNode::getColor(video::SColor & topColor, video::SColor & bottomColor) const
{
	topColor = ColorTop;
	bottomColor = ColorBottom;
}


} // end namespace scene
} // end namespace irr

