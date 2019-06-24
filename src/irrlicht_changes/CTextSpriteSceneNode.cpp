// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CTextSpriteSceneNode.h"
#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "ICameraSceneNode.h"
#include "IGUISpriteBank.h"
#include "SMeshBuffer.h"
#include "log.h"
#include "util/string.h"
#include <map>
#include <stdlib.h>

namespace irr
{
namespace scene
{
//! constructor
CTextSpriteSceneNode::CTextSpriteSceneNode(ISceneNode *parent, ISceneManager *mgr, s32 id,
		gui::IGUIFont *font,const wchar_t *text,
		const v3f &position, const core::dimension2d<f32> &size,
		const video::SColor colorTop, const video::SColor bottomColor,
		const bool background, const video::SColor &backgroundColor, 
		const video::SColor &borderColor, const f32 border,
		const f32 xPadding, const f32 yPadding,
		const f32 xOffset, const f32 yOffset, 
		const f32 spacing, const f32 baseOffset)
	: IBillboardTextSceneNode(parent, mgr, id, position),
		LineCount(1), Font(0), TopColor(colorTop), BottomColor(bottomColor), 
		Background(background), BackgroundColor(backgroundColor), 
		BorderColor(borderColor), Border(border), 
		XPadding(xPadding), YPadding(yPadding), 
		XOffset(xOffset), YOffset(yOffset), 
		Spacing(spacing), BaseOffset(baseOffset), 
		Mesh(0)
{
#ifdef _DEBUG
	setDebugName("CTextSpriteSceneNode");
#endif

	Material.MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	Material.MaterialTypeParam = 1.f / 255.f;
	Material.BackfaceCulling = false;
	Material.Lighting = false;
	Material.ZBuffer = video::ECFN_LESSEQUAL;
	Material.ZWriteEnable = false;

	if (font) {
		// doesn't support other font types
		if (font->getType() == gui::EGFT_BITMAP) {
			Font = (gui::IGUIFontBitmap *)font;
			Font->grab();

			// mesh with one buffer per texture
			Mesh = new SMesh();
			for (u32 i = 0; i < Font->getSpriteBank()->getTextureCount(); ++i) {
				SMeshBuffer *mb = new SMeshBuffer();
				mb->Material = Material;
				mb->Material.setTexture(0, Font->getSpriteBank()->getTexture(i));
				Mesh->addMeshBuffer(mb);
				mb->drop();
			}
		} else {
			warningstream << "Sorry, CTextSpriteSceneNode does not support this font type";
		}
	}

	setSize(size);
	setText(text);
	setAutomaticCulling(scene::EAC_BOX);
}



CTextSpriteSceneNode::~CTextSpriteSceneNode()
{
	if (Font)
		Font->drop();
	if (Mesh)
		Mesh->drop();
}


//! sets the text string
void CTextSpriteSceneNode::setText(const wchar_t *text)
{
	if (!Mesh || OldText == text)
		return;

	OldText = text;
	Text = "";
	Width = 0.f;
	Height = 0.f;
	core::array<s32> charLineBreaks;
	core::array<video::SColor> charTopColors;
	core::array<video::SColor> charBottomColors;
	core::array<f32> charScales;
	LineCount = 0.f;
	f32 lineBreaks = 0.f;
	video::SColor topColor = TopColor;
	video::SColor bottomColor = BottomColor;
	f32 scale = 1.f;
	f32 lineScale = 1.f;

	for (const wchar_t *c=text; *c; ++c) {
		if (*c == L'\n') {
			LineCount += lineScale;
			lineBreaks += 1.f;
		} else {
			if (*c == L'{') {
				++c;
				std::string color_string;

				while (*c) {
					if (*c == L'}')
						break;
					color_string += char(*c);
					++c;
				}

				std::size_t slash = color_string.find('/');

				if (slash == std::string::npos) {
					parseColor(color_string, topColor, scale);
					parseColor(color_string, bottomColor, scale);
				} else {
					parseColor(color_string.substr(0,slash), topColor, scale);
					parseColor(color_string.substr(slash + 1), bottomColor, scale);
				}
				
				if (scale > lineScale)
					lineScale = scale;
				
				continue;
			} else if (*c == L'\\' && c[1]) {
				++c;
			}
			
			Text += *c;
			
			charLineBreaks.push_back(lineBreaks);
			charTopColors.push_back(topColor);
			charBottomColors.push_back(bottomColor);
			charScales.push_back(scale);
			
			if (lineBreaks > 0.f)
				lineScale = scale;
			
			lineBreaks = 0.f;
		}
	}
	
	LineCount += lineScale;
	Symbol.clear();

	// clear mesh
	for (u32 j = 0; j < Mesh->getMeshBufferCount(); ++j) {
		((SMeshBuffer*)Mesh->getMeshBuffer(j))->Indices.clear();
		((SMeshBuffer*)Mesh->getMeshBuffer(j))->Vertices.clear();
	}

	if (!Font)
		return;

	const core::array< core::rect<s32> > &sourceRects = Font->getSpriteBank()->getPositions();
	const core::array< gui::SGUISprite > &sprites = Font->getSpriteBank()->getSprites();

	f32 dim[2];
	f32 tex[4];
	
	f32 xPosition = 0.f;
	f32 yPosition = 0.f;
	f32 lineHeight = 0.f;
	f32 lineBaseHeight = 0.f;
	s32 i;
	
	for (i = 0; i < (s32)Text.size(); ++i) {
		SSymbolInfo info;

		u32 spriteno = Font->getSpriteNoFromChar(&Text.c_str()[i]);
		u32 rectno = sprites[spriteno].Frames[0].rectNumber;
		u32 texno = sprites[spriteno].Frames[0].textureNumber;

		dim[0] = core::reciprocal((f32)Font->getSpriteBank()->getTexture(texno)->getSize().Width);
		dim[1] = core::reciprocal((f32)Font->getSpriteBank()->getTexture(texno)->getSize().Height);

		const core::rect<s32> &s = sourceRects[rectno];

		// add space for letter to buffer
		SMeshBuffer* buf = (SMeshBuffer *)Mesh->getMeshBuffer(texno);
		u32 firstInd = buf->Indices.size();
		u32 firstVert = buf->Vertices.size();
		buf->Indices.set_used(firstInd + 6);
		buf->Vertices.set_used(firstVert + 4);

		tex[0] = (s.LowerRightCorner.X * dim[0]) + 0.5f * dim[0]; // half pixel
		tex[1] = (s.LowerRightCorner.Y * dim[1]) + 0.5f * dim[1];
		tex[2] = (s.UpperLeftCorner.Y  * dim[1]) - 0.5f * dim[1];
		tex[3] = (s.UpperLeftCorner.X  * dim[0]) - 0.5f * dim[0];

		buf->Vertices[firstVert + 0].TCoords.set(tex[0], tex[1]);
		buf->Vertices[firstVert + 1].TCoords.set(tex[0], tex[2]);
		buf->Vertices[firstVert + 2].TCoords.set(tex[3], tex[2]);
		buf->Vertices[firstVert + 3].TCoords.set(tex[3], tex[1]);

		buf->Vertices[firstVert + 0].Color = charBottomColors[i];
		buf->Vertices[firstVert + 3].Color = charBottomColors[i];
		buf->Vertices[firstVert + 1].Color = charTopColors[i];
		buf->Vertices[firstVert + 2].Color = charTopColors[i];

		buf->Indices[firstInd + 0] = (u16)firstVert + 0;
		buf->Indices[firstInd + 1] = (u16)firstVert + 2;
		buf->Indices[firstInd + 2] = (u16)firstVert + 1;
		buf->Indices[firstInd + 3] = (u16)firstVert + 0;
		buf->Indices[firstInd + 4] = (u16)firstVert + 3;
		buf->Indices[firstInd + 5] = (u16)firstVert + 2;

		info.Scale = charScales[i];
		info.Kerning = 0.f;
		info.Width = (f32)s.getWidth() * info.Scale;
		info.Height = (f32)s.getHeight() * info.Scale;
		info.BaseHeight = BaseOffset * info.Height;
		info.bufNo = texno;
		info.firstInd = firstInd;
		info.firstVert = firstVert;
		info.LineBreaks = charLineBreaks[i];
		info.TopColor = charTopColors[i];
		info.BottomColor = charBottomColors[i];
			
		if (info.LineBreaks > 0.f) {			
			xPosition = 0.f;
			yPosition += lineHeight * info.LineBreaks;
			lineHeight = info.Height;
			lineBaseHeight = info.BaseHeight;
		} else if (i > 0) {
			info.Kerning = Spacing * 2.f;
			
			SSymbolInfo &priorInfo = Symbol[i - 1];
			
			if (info.Scale <= priorInfo.Scale) 
				info.Kerning *= info.Scale;
			else
				info.Kerning *= priorInfo.Scale;
		}
				
		xPosition += info.Kerning;

		info.XPosition = xPosition;
		info.YPosition = yPosition;
		info.LineHeight = lineHeight;
		info.LineBaseHeight = lineBaseHeight;
		
		xPosition += info.Width;
		
		if (info.XPosition + info.Width > Width)
			Width = info.XPosition + info.Width;
		
		if (info.YPosition + info.Height > Height)
			Height = info.YPosition + info.Height;
			
		if (info.Height > lineHeight)
			lineHeight = info.Height;
			
		if (info.BaseHeight > lineBaseHeight)
			lineBaseHeight = info.BaseHeight;

		Symbol.push_back(info);		
	}
	
	if (xPosition > Width)
		Width = xPosition;
		
	if (yPosition > Height)
		Height = yPosition;
		
	lineHeight = 0.f;
	lineBaseHeight = 0.f;
	
	for (i = Symbol.size() - 1; i >= 0; --i)
	{
		SSymbolInfo &info = Symbol[i];
		
		if (lineHeight == 0.f) {
			lineHeight = info.LineHeight;
			lineBaseHeight = info.LineBaseHeight;
		} else {
			info.LineHeight = lineHeight;
			info.LineBaseHeight = lineBaseHeight;
		}
		
		if (info.LineBreaks > 0.f) {
			lineHeight = 0.f;
			lineBaseHeight = 0.f;
		}
			
		info.YPosition += info.LineHeight - info.Height -
				info.LineBaseHeight + info.BaseHeight;
	}
	
	resize();
}

//! resize the billboard
void CTextSpriteSceneNode::resize()
{
	if (!IsVisible || !Font || !Mesh)
		return;

	ICameraSceneNode* camera = SceneManager->getActiveCamera();
	if (!camera)
		return;

	Size.Width = Width * Size.Height / Height;
		
	//const core::matrix4 &m = camera->getViewFrustum()->Matrices[ video::ETS_VIEW ];

	// make billboard look to camera
	v3f pos = getAbsolutePosition();

	v3f campos = camera->getAbsolutePosition();
	v3f target = camera->getTarget();
	v3f up = camera->getUpVector();
	v3f view = target - campos;
	view.normalize();

	v3f horizontal = up.crossProduct(view);
	if (horizontal.getLength() == 0)
		horizontal.set(up.Y, up.X, up.Z);

	horizontal.normalize();
	horizontal *= Size.Height / Height;

	v3f vertical = horizontal.crossProduct(view);
	vertical.normalize();
	vertical *= Size.Height / Height;
	
	view *= -1.f;
	
	f32 x_offset = Size.Height / LineCount * XOffset;
	f32 y_offset = Size.Height / LineCount * YOffset;
	pos = pos + (horizontal * x_offset) + (vertical * y_offset);
	
	u32 i;
	for (i = 0; i < Symbol.size(); ++i) {
		SSymbolInfo &info = Symbol[i];
		
		SMeshBuffer* buf = (SMeshBuffer*)Mesh->getMeshBuffer(info.bufNo);

		buf->Vertices[info.firstVert + 0].Normal = view;
		buf->Vertices[info.firstVert + 1].Normal = view;
		buf->Vertices[info.firstVert + 2].Normal = view;
		buf->Vertices[info.firstVert + 3].Normal = view;
		
		f32 left = info.XPosition - Width * 0.5;
		f32 top = info.YPosition - Height * 0.5;
		f32 right = left + info.Width;
		f32 bottom = top + info.Height;

		buf->Vertices[info.firstVert + 0].Pos = pos + (horizontal * right) + (vertical * bottom);
		buf->Vertices[info.firstVert + 1].Pos = pos + (horizontal * right) + (vertical * top);
		buf->Vertices[info.firstVert + 2].Pos = pos + (horizontal * left) + (vertical * top);
		buf->Vertices[info.firstVert + 3].Pos = pos + (horizontal * left) + (vertical * bottom);
		
		buf->Vertices[info.firstVert + 0].Color = info.BottomColor;
		buf->Vertices[info.firstVert + 3].Color = info.BottomColor;
		buf->Vertices[info.firstVert + 1].Color = info.TopColor;
		buf->Vertices[info.firstVert + 2].Color = info.TopColor;
	}

	// make bounding box
	for (i = 0; i < Mesh->getMeshBufferCount(); ++i)
		Mesh->getMeshBuffer(i)->recalculateBoundingBox();

	Mesh->recalculateBoundingBox();
	BBox = Mesh->getBoundingBox();
	core::matrix4 mat(getAbsoluteTransformation(), core::matrix4::EM4CONST_INVERSE);
	mat.transformBoxEx(BBox);
}

//! pre render event
void CTextSpriteSceneNode::OnAnimate(u32 timeMs)
{
	ISceneNode::OnAnimate(timeMs);
	resize();
}

void CTextSpriteSceneNode::OnRegisterSceneNode()
{
	SceneManager->registerNodeForRendering(this, ESNRP_TRANSPARENT);
	ISceneNode::OnRegisterSceneNode();
}

//! render background
void CTextSpriteSceneNode::renderBackground()
{
	video::IVideoDriver *driver = SceneManager->getVideoDriver();
	ICameraSceneNode *camera = SceneManager->getActiveCamera();

	if (!camera || !driver)
		return;
		
	video::S3DVertex vertices[4];
	u16 indices[6];
	f32 border = Size.Height / LineCount * Border;
	f32 x_padding = Size.Height / LineCount * XPadding;
	f32 y_padding = Size.Height / LineCount * YPadding;
	f32 width = Size.Width + x_padding * 2;
	f32 height = Size.Height + y_padding * 2;
	
	indices[0] = 0;
	indices[1] = 2;
	indices[2] = 1;
	indices[3] = 0;
	indices[4] = 3;
	indices[5] = 2;

	vertices[0].TCoords.set(1.f, 1.f);
	vertices[0].Color = BackgroundColor;

	vertices[1].TCoords.set(1.f, 0.f);
	vertices[1].Color = BackgroundColor;

	vertices[2].TCoords.set(0.f, 0.f);
	vertices[2].Color = BackgroundColor;

	vertices[3].TCoords.set(0.f, 1.f);
	vertices[3].Color = BackgroundColor;
	
	// make billboard look to camera

	v3f pos = getAbsolutePosition();

	v3f campos = camera->getAbsolutePosition();
	v3f target = camera->getTarget();
	v3f up = camera->getUpVector();
	v3f view = target - campos;
	view.normalize();

	v3f horizontal = up.crossProduct(view);
	if (horizontal.getLength() == 0)
		horizontal.set(up.Y, up.X, up.Z);

	horizontal.normalize();
	v3f borderHorizontal = horizontal * border;
	horizontal *= 0.5f * width;

	// pointing down!
	v3f vertical = horizontal.crossProduct(view);
	vertical.normalize();
	v3f borderVertical = vertical * border;
	vertical *= 0.5f * height;

	view *= -1.f;

	for (s32 i = 0; i < 4; ++i)
		vertices[i].Normal = view;

	/* Vertices are:
	2--1
	|\ |
	| \|
	3--0
	*/
	
	driver->setTransform(video::ETS_WORLD, core::IdentityMatrix);
	driver->setMaterial(Material);
	
	// background
	vertices[2].Pos = pos - horizontal - vertical;
	vertices[1].Pos = pos + horizontal - vertical;
	vertices[3].Pos = pos - horizontal + vertical;
	vertices[0].Pos = pos + horizontal + vertical;
	driver->drawIndexedTriangleList(vertices, 4, indices, 2);
	
	vertices[0].Color = BorderColor;
	vertices[1].Color = BorderColor;
	vertices[2].Color = BorderColor;
	vertices[3].Color = BorderColor;
	
	// top border
	vertices[2].Pos = pos - horizontal - borderHorizontal - vertical - borderVertical;
	vertices[1].Pos = pos + horizontal + borderHorizontal - vertical - borderVertical;
	vertices[3].Pos = pos - horizontal - borderHorizontal - vertical;
	vertices[0].Pos = pos + horizontal + borderHorizontal - vertical;
	driver->drawIndexedTriangleList(vertices, 4, indices, 2);
	
	// bottom border
	vertices[2].Pos = pos - horizontal - borderHorizontal + vertical;
	vertices[1].Pos = pos + horizontal + borderHorizontal + vertical;
	vertices[3].Pos = pos - horizontal - borderHorizontal + vertical + borderVertical;
	vertices[0].Pos = pos + horizontal + borderHorizontal + vertical + borderVertical;
	driver->drawIndexedTriangleList(vertices, 4, indices, 2);
	
	// left border
	vertices[2].Pos = pos - horizontal - borderHorizontal - vertical;
	vertices[1].Pos = pos - horizontal - vertical;
	vertices[3].Pos = pos - horizontal - borderHorizontal + vertical;
	vertices[0].Pos = pos - horizontal + vertical;
	driver->drawIndexedTriangleList(vertices, 4, indices, 2);
	
	// right border
	vertices[2].Pos = pos + horizontal - vertical;
	vertices[1].Pos = pos + horizontal + borderHorizontal - vertical;
	vertices[3].Pos = pos + horizontal + vertical;
	vertices[0].Pos = pos + horizontal + borderHorizontal + vertical;
	driver->drawIndexedTriangleList(vertices, 4, indices, 2);
}


//! render
void CTextSpriteSceneNode::render()
{
	if (!Mesh)
		return;

	if (Background)
		renderBackground();
	
	video::IVideoDriver *driver = SceneManager->getVideoDriver();

	// draw
	core::matrix4 mat;
	driver->setTransform(video::ETS_WORLD, mat);

	for (u32 i = 0; i < Mesh->getMeshBufferCount(); ++i) {
		driver->setMaterial(Mesh->getMeshBuffer(i)->getMaterial());
		driver->drawMeshBuffer(Mesh->getMeshBuffer(i));
	}

	if (DebugDataVisible & scene::EDS_BBOX) {
		driver->setTransform(video::ETS_WORLD, AbsoluteTransformation);
		video::SMaterial m;
		m.Lighting = false;
		driver->setMaterial(m);
		driver->draw3DBox(BBox, video::SColor(0, 208, 195, 152));
	}
}


//! returns the axis aligned bounding box of this node
const core::aabbox3d<f32> &CTextSpriteSceneNode::getBoundingBox() const
{
	return BBox;
}


//! sets the size of the billboard
void CTextSpriteSceneNode::setSize(const core::dimension2d<f32> &size)
{
	Size = size;

	if (Size.Width == 0.f)
		Size.Width = 1.f;

	if (Size.Height == 0.f)
		Size.Height = 1.f;

	//f32 avg = (size.Width + size.Height)/6;
	//BBox.MinEdge.set(-avg,-avg,-avg);
	//BBox.MaxEdge.set(avg,avg,avg);
}


video::SMaterial& CTextSpriteSceneNode::getMaterial(u32 i)
{
	if (Mesh && Mesh->getMeshBufferCount() > i)
		return Mesh->getMeshBuffer(i)->getMaterial();
	return Material;
}


//! returns amount of materials used by this scene node.
u32 CTextSpriteSceneNode::getMaterialCount() const
{
	if (Mesh)
		return Mesh->getMeshBufferCount();
	return 0;
}


//! gets the size of the billboard
const core::dimension2d<f32> &CTextSpriteSceneNode::getSize() const
{
	return Size;
}


//! sets the color of the text
void CTextSpriteSceneNode::setTextColor(video::SColor color)
{
	TopColor = color;
	BottomColor = color;
}

//! Set the color of all vertices of the billboard
//! \param overallColor: the color to set
void CTextSpriteSceneNode::setColor(const video::SColor &overallColor)
{
	if (!Mesh)
		return;

	for (u32 i = 0; i != Text.size(); ++i) {
		const SSymbolInfo &info = Symbol[i];
		SMeshBuffer *buf = (SMeshBuffer *)Mesh->getMeshBuffer(info.bufNo);
		buf->Vertices[info.firstVert + 0].Color = overallColor;
		buf->Vertices[info.firstVert + 1].Color = overallColor;
		buf->Vertices[info.firstVert + 2].Color = overallColor;
		buf->Vertices[info.firstVert + 3].Color = overallColor;
	}
}


//! Set the color of the top and bottom vertices of the billboard
//! \param topColor: the color to set the top vertices
//! \param bottomColor: the color to set the bottom vertices
void CTextSpriteSceneNode::setColor(
	const video::SColor &topColor, const video::SColor &bottomColor)
{
	if (!Mesh)
		return;

	BottomColor = bottomColor;
	TopColor = topColor;

	for (u32 i = 0; i != Text.size (); ++i) {
		const SSymbolInfo &info = Symbol[i];
		SMeshBuffer *buf = (SMeshBuffer *)Mesh->getMeshBuffer(info.bufNo);
		buf->Vertices[info.firstVert + 0].Color = BottomColor;
		buf->Vertices[info.firstVert + 3].Color = BottomColor;
		buf->Vertices[info.firstVert + 1].Color = TopColor;
		buf->Vertices[info.firstVert + 2].Color = TopColor;
	}
}


//! Gets the color of the top and bottom vertices of the billboard
//! \param topColor: stores the color of the top vertices
//! \param bottomColor: stores the color of the bottom vertices
void CTextSpriteSceneNode::getColor(
	video::SColor &topColor, video::SColor &bottomColor) const
{
	topColor = TopColor;
	bottomColor = BottomColor;
}

//! Parses an hexadecimal color
void CTextSpriteSceneNode::parseColor(const std::string &color_string,
	video::SColor &color, f32 &scale)
{	
	u32 rgbcolor = 0;
	
	if (color_string.length() > 0) {
		const char *c=color_string.c_str();
		
		if (*c >= '0' && *c <= '9') {
			scale = atof(c);
			return;
		} else if (*c == '#') {
			++c;
	
			while (*c)
			{
				u32 digit = 0;
				if (*c >= '0' && *c <= '9')
					digit = *c - '0';
				else if (*c >= 'a' && *c <= 'f')
					digit = (*c - 'a') + 10;
				else if (*c >= 'A' && *c <= 'F')
					digit = (*c - 'A') + 10;
					
				rgbcolor = (rgbcolor<<4) | digit;
				++c;
			}
			
			if (color_string.length() <= 5) {
				rgbcolor = ((rgbcolor & 0xF000) << 16) | ((rgbcolor & 0xF000) << 12) | 
					   ((rgbcolor & 0xF00) <<  12) | ((rgbcolor & 0xF00)  <<  8) | 
					   ((rgbcolor & 0x0F0) <<   8) | ((rgbcolor & 0x0F0)  <<  4) | 
					   ((rgbcolor & 0x00F) <<   4) |  (rgbcolor & 0x00F);
					
				if (color_string.length() <= 3)
					rgbcolor |= 0xFF000000;
			} else if (color_string.length() <= 6) {
					rgbcolor |= 0xFF000000;
			}
		} else {	
			std::map<const std::string, u32>::const_iterator found_color;
			found_color = NAMED_COLORS.colors.find(color_string);
			
			if (found_color != NAMED_COLORS.colors.end())
				rgbcolor = 0xFF000000 | found_color->second;
		}
	}

	color.color = rgbcolor;
}

//! Adds a text scene node, which uses billboards
CTextSpriteSceneNode *CTextSpriteSceneNode::addBillboardTextSceneNode(
		gui::IGUIFont* font, const wchar_t *text, 
		gui::IGUIEnvironment *environment, ISceneManager *smgr, ISceneNode *parent,
		const core::dimension2d<f32> &size,
		const v3f &position, s32 id,
		const video::SColor colorTop, const video::SColor colorBottom,
		const bool background, const video::SColor &backgroundColor, 
		const video::SColor &borderColor, const f32 border,
		const f32 xPadding, const f32 yPadding,
		const f32 xOffset, const f32 yOffset, 
		const f32 spacing, const f32 baseOffset)
{
	if (!font && environment)
		font = environment->getBuiltInFont();

	if (!font)
		return 0;

	if (!parent)
		parent = smgr->getRootSceneNode();

	CTextSpriteSceneNode *node = new CTextSpriteSceneNode(
		parent, smgr, id, font, text, position, size,
		colorTop, colorBottom, background, backgroundColor, borderColor, border,
		xPadding, yPadding, xOffset, yOffset, spacing, baseOffset);

	node->drop();
	return node;

}
} // end namespace scene
} // end namespace irr
