// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CGUIFont.h"
#ifdef _IRR_COMPILE_WITH_GUI_

#include "os.h"
#include "IGUIEnvironment.h"
#include "IXMLReader.h"
#include "IReadFile.h"
#include "IVideoDriver.h"
#include "IGUISpriteBank.h"

namespace irr
{
namespace gui
{

//! constructor
CGUIFont::CGUIFont(IGUIEnvironment *env, const io::path& filename)
: Driver(0), SpriteBank(0), Environment(env), WrongCharacter(0),
	MaxHeight(0), GlobalKerningWidth(0), GlobalKerningHeight(0)
{
	#ifdef _DEBUG
	setDebugName("CGUIFont");
	#endif

	if (Environment)
	{
		// don't grab environment, to avoid circular references
		Driver = Environment->getVideoDriver();

		SpriteBank = Environment->getSpriteBank(filename);
		if (!SpriteBank)	// could be default-font which has no file
			SpriteBank = Environment->addEmptySpriteBank(filename);
		if (SpriteBank)
			SpriteBank->grab();
	}

	if (Driver)
		Driver->grab();

	setInvisibleCharacters ( L" " );
}


//! destructor
CGUIFont::~CGUIFont()
{
	if (Driver)
		Driver->drop();

	if (SpriteBank)
	{
		SpriteBank->drop();
		// TODO: spritebank still exists in gui-environment and should be removed here when it's
		// reference-count is 1. Just can't do that from here at the moment.
		// But spritebank would not be able to drop textures anyway because those are in texture-cache
		// where they can't be removed unless materials start reference-couting 'em.
	}
}


//! loads a font file from xml
bool CGUIFont::load(io::IXMLReader* xml)
{
	if (!SpriteBank)
		return false;

	SpriteBank->clear();

	while (xml->read())
	{
		if (io::EXN_ELEMENT == xml->getNodeType())
		{
			if (core::stringw(L"Texture") == xml->getNodeName())
			{
				// add a texture
				core::stringc fn = xml->getAttributeValue(L"filename");
				u32 i = (u32)xml->getAttributeValueAsInt(L"index");
				core::stringw alpha = xml->getAttributeValue(L"hasAlpha");

				while (i+1 > SpriteBank->getTextureCount())
					SpriteBank->addTexture(0);

				// disable mipmaps+filtering
				bool mipmap = Driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);
				Driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);

				// load texture
				SpriteBank->setTexture(i, Driver->getTexture(fn));

				// set previous mip-map+filter state
				Driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, mipmap);

				// couldn't load texture, abort.
				if (!SpriteBank->getTexture(i))
				{
					os::Printer::log("Unable to load all textures in the font, aborting", ELL_ERROR);
					_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
					return false;
				}
				else
				{
					// colorkey texture rather than alpha channel?
					if (alpha == core::stringw("false"))
						Driver->makeColorKeyTexture(SpriteBank->getTexture(i), core::position2di(0,0));
				}
			}
			else if (core::stringw(L"c") == xml->getNodeName())
			{
				// adding a character to this font
				SFontArea a;
				SGUISpriteFrame f;
				SGUISprite s;
				core::rect<s32> rectangle;

				a.underhang		= xml->getAttributeValueAsInt(L"u");
				a.overhang		= xml->getAttributeValueAsInt(L"o");
				a.spriteno		= SpriteBank->getSprites().size();
				s32 texno		= xml->getAttributeValueAsInt(L"i");

				// parse rectangle
				core::stringc rectstr	= xml->getAttributeValue(L"r");
				wchar_t ch		= xml->getAttributeValue(L"c")[0];

				const c8 *c = rectstr.c_str();
				s32 val;
				val = 0;
				while (*c >= '0' && *c <= '9')
				{
					val *= 10;
					val += *c - '0';
					c++;
				}
				rectangle.UpperLeftCorner.X = val;
				while (*c == L' ' || *c == L',') c++;

				val = 0;
				while (*c >= '0' && *c <= '9')
				{
					val *= 10;
					val += *c - '0';
					c++;
				}
				rectangle.UpperLeftCorner.Y = val;
				while (*c == L' ' || *c == L',') c++;

				val = 0;
				while (*c >= '0' && *c <= '9')
				{
					val *= 10;
					val += *c - '0';
					c++;
				}
				rectangle.LowerRightCorner.X = val;
				while (*c == L' ' || *c == L',') c++;

				val = 0;
				while (*c >= '0' && *c <= '9')
				{
					val *= 10;
					val += *c - '0';
					c++;
				}
				rectangle.LowerRightCorner.Y = val;

				CharacterMap.insert(ch,Areas.size());

				// make frame
				f.rectNumber = SpriteBank->getPositions().size();
				f.textureNumber = texno;

				// add frame to sprite
				s.Frames.push_back(f);
				s.frameTime = 0;

				// add rectangle to sprite bank
				SpriteBank->getPositions().push_back(rectangle);
				a.width = rectangle.getWidth();

				// add sprite to sprite bank
				SpriteBank->getSprites().push_back(s);

				// add character to font
				Areas.push_back(a);
			}
		}
	}

	// set bad character
	WrongCharacter = getAreaFromCharacter(L' ');

	setMaxHeight();

	return true;
}


void CGUIFont::setMaxHeight()
{
	if ( !SpriteBank )
		return;

	MaxHeight = 0;
	s32 t;

	core::array< core::rect<s32> >& p = SpriteBank->getPositions();

	for (u32 i=0; i<p.size(); ++i)
	{
		t = p[i].getHeight();
		if (t>MaxHeight)
			MaxHeight = t;
	}

}


//! loads a font file, native file needed, for texture parsing
bool CGUIFont::load(io::IReadFile* file)
{
	if (!Driver)
		return false;

	return loadTexture(Driver->createImageFromFile(file),
				file->getFileName());
}


//! loads a font file, native file needed, for texture parsing
bool CGUIFont::load(const io::path& filename)
{
	if (!Driver)
		return false;

	return loadTexture(Driver->createImageFromFile( filename ),
				filename);
}


//! load & prepare font from ITexture
bool CGUIFont::loadTexture(video::IImage* image, const io::path& name)
{
	if (!image || !SpriteBank)
		return false;

	s32 lowerRightPositions = 0;

	video::IImage* tmpImage=image;
	bool deleteTmpImage=false;
	switch(image->getColorFormat())
	{
	case video::ECF_R5G6B5:
		tmpImage =  Driver->createImage(video::ECF_A1R5G5B5,image->getDimension());
		image->copyTo(tmpImage);
		deleteTmpImage=true;
		break;
	case video::ECF_A1R5G5B5:
	case video::ECF_A8R8G8B8:
		break;
	case video::ECF_R8G8B8:
		tmpImage = Driver->createImage(video::ECF_A8R8G8B8,image->getDimension());
		image->copyTo(tmpImage);
		deleteTmpImage=true;
		break;
	default:
		os::Printer::log("Unknown texture format provided for CGUIFont::loadTexture", ELL_ERROR);
		return false;
	}
	readPositions(tmpImage, lowerRightPositions);

	WrongCharacter = getAreaFromCharacter(L' ');

	// output warnings
	if (!lowerRightPositions || !SpriteBank->getSprites().size())
		os::Printer::log("Either no upper or lower corner pixels in the font file. If this font was made using the new font tool, please load the XML file instead. If not, the font may be corrupted.", ELL_ERROR);
	else
	if (lowerRightPositions != (s32)SpriteBank->getPositions().size())
		os::Printer::log("The amount of upper corner pixels and the lower corner pixels is not equal, font file may be corrupted.", ELL_ERROR);

	bool ret = ( !SpriteBank->getSprites().empty() && lowerRightPositions );

	if ( ret )
	{
		bool flag[2];
		flag[0] = Driver->getTextureCreationFlag ( video::ETCF_ALLOW_NON_POWER_2 );
		flag[1] = Driver->getTextureCreationFlag ( video::ETCF_CREATE_MIP_MAPS );

		Driver->setTextureCreationFlag(video::ETCF_ALLOW_NON_POWER_2, true);
		Driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false );

		SpriteBank->addTexture(Driver->addTexture(name, tmpImage));

		Driver->setTextureCreationFlag(video::ETCF_ALLOW_NON_POWER_2, flag[0] );
		Driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, flag[1] );
	}
	if (deleteTmpImage)
		tmpImage->drop();
	image->drop();

	setMaxHeight();

	return ret;
}


void CGUIFont::readPositions(video::IImage* image, s32& lowerRightPositions)
{
	if (!SpriteBank )
		return;

	const core::dimension2d<u32> size = image->getDimension();

	video::SColor colorTopLeft = image->getPixel(0,0);
	colorTopLeft.setAlpha(255);
	image->setPixel(0,0,colorTopLeft);
	video::SColor colorLowerRight = image->getPixel(1,0);
	video::SColor colorBackGround = image->getPixel(2,0);
	video::SColor colorBackGroundTransparent = 0;

	image->setPixel(1,0,colorBackGround);

	// start parsing

	core::position2d<s32> pos(0,0);
	for (pos.Y=0; pos.Y<(s32)size.Height; ++pos.Y)
	{
		for (pos.X=0; pos.X<(s32)size.Width; ++pos.X)
		{
			const video::SColor c = image->getPixel(pos.X, pos.Y);
			if (c == colorTopLeft)
			{
				image->setPixel(pos.X, pos.Y, colorBackGroundTransparent);
				SpriteBank->getPositions().push_back(core::rect<s32>(pos, pos));
			}
			else
			if (c == colorLowerRight)
			{
				// too many lower right points
				if (SpriteBank->getPositions().size()<=(u32)lowerRightPositions)
				{
					lowerRightPositions = 0;
					return;
				}

				image->setPixel(pos.X, pos.Y, colorBackGroundTransparent);
				SpriteBank->getPositions()[lowerRightPositions].LowerRightCorner = pos;
				// add frame to sprite bank
				SGUISpriteFrame f;
				f.rectNumber = lowerRightPositions;
				f.textureNumber = 0;
				SGUISprite s;
				s.Frames.push_back(f);
				s.frameTime = 0;
				SpriteBank->getSprites().push_back(s);
				// add character to font
				SFontArea a;
				a.overhang = 0;
				a.underhang = 0;
				a.spriteno = lowerRightPositions;
				a.width = SpriteBank->getPositions()[lowerRightPositions].getWidth();
				Areas.push_back(a);
				// map letter to character
				wchar_t ch = (wchar_t)(lowerRightPositions + 32);
				CharacterMap.set(ch, lowerRightPositions);

				++lowerRightPositions;
			}
			else
			if (c == colorBackGround)
				image->setPixel(pos.X, pos.Y, colorBackGroundTransparent);
		}
	}
}


//! set an Pixel Offset on Drawing ( scale position on width )
void CGUIFont::setKerningWidth(s32 kerning)
{
	GlobalKerningWidth = kerning;
}


//! set an Pixel Offset on Drawing ( scale position on width )
s32 CGUIFont::getKerningWidth(const wchar_t* thisLetter, const wchar_t* previousLetter) const
{
	s32 ret = GlobalKerningWidth;

	if (thisLetter)
	{
		ret += Areas[getAreaFromCharacter(*thisLetter)].overhang;

		if (previousLetter)
		{
			ret += Areas[getAreaFromCharacter(*previousLetter)].underhang;
		}
	}

	return ret;
}


//! set an Pixel Offset on Drawing ( scale position on height )
void CGUIFont::setKerningHeight(s32 kerning)
{
	GlobalKerningHeight = kerning;
}


//! set an Pixel Offset on Drawing ( scale position on height )
s32 CGUIFont::getKerningHeight () const
{
	return GlobalKerningHeight;
}


//! returns the sprite number from a given character
u32 CGUIFont::getSpriteNoFromChar(const wchar_t *c) const
{
	return Areas[getAreaFromCharacter(*c)].spriteno;
}


s32 CGUIFont::getAreaFromCharacter(const wchar_t c) const
{
	core::map<wchar_t, s32>::Node* n = CharacterMap.find(c);
	if (n)
		return n->getValue();
	else
		return WrongCharacter;
}

void CGUIFont::setInvisibleCharacters( const wchar_t *s )
{
	Invisible = s;
}


//! returns the dimension of text
core::dimension2d<u32> CGUIFont::getDimension(const wchar_t* text) const
{
	core::dimension2d<u32> dim(0, 0);
	core::dimension2d<u32> thisLine(0, MaxHeight);

	for (const wchar_t* p = text; *p; ++p)
	{
		bool lineBreak=false;
		if (*p == L'\r') // Mac or Windows breaks
		{
			lineBreak = true;
			if (p[1] == L'\n') // Windows breaks
				++p;
		}
		else if (*p == L'\n') // Unix breaks
		{
			lineBreak = true;
		}
		if (lineBreak)
		{
			dim.Height += thisLine.Height;
			if (dim.Width < thisLine.Width)
				dim.Width = thisLine.Width;
			thisLine.Width = 0;
			continue;
		}

		const SFontArea &area = Areas[getAreaFromCharacter(*p)];

		thisLine.Width += area.underhang;
		thisLine.Width += area.width + area.overhang + GlobalKerningWidth;
	}

	dim.Height += thisLine.Height;
	if (dim.Width < thisLine.Width)
		dim.Width = thisLine.Width;

	return dim;
}

//! draws some text and clips it to the specified rectangle if wanted
void CGUIFont::draw(const core::stringw& text, const core::rect<s32>& position,
					video::SColor color,
					bool hcenter, bool vcenter, const core::rect<s32>* clip
				)
{
	if (!Driver || !SpriteBank)
		return;

	core::dimension2d<s32> textDimension;	// NOTE: don't make this u32 or the >> later on can fail when the dimension width is < position width
	core::position2d<s32> offset = position.UpperLeftCorner;

	if (hcenter || vcenter || clip)
		textDimension = getDimension(text.c_str());

	if (hcenter)
		offset.X += (position.getWidth() - textDimension.Width) >> 1;

	if (vcenter)
		offset.Y += (position.getHeight() - textDimension.Height) >> 1;

	if (clip)
	{
		core::rect<s32> clippedRect(offset, textDimension);
		clippedRect.clipAgainst(*clip);
		if (!clippedRect.isValid())
			return;
	}

	core::array<u32> indices(text.size());
	core::array<core::position2di> offsets(text.size());

	for(u32 i = 0;i < text.size();i++)
	{
		wchar_t c = text[i];

		bool lineBreak=false;
		if ( c == L'\r') // Mac or Windows breaks
		{
			lineBreak = true;
			if ( text[i + 1] == L'\n') // Windows breaks
				c = text[++i];
		}
		else if ( c == L'\n') // Unix breaks
		{
			lineBreak = true;
		}

		if (lineBreak)
		{
			offset.Y += MaxHeight;
			offset.X = position.UpperLeftCorner.X;

			if ( hcenter )
			{
				offset.X += (position.getWidth() - textDimension.Width) >> 1;
			}
			continue;
		}

		SFontArea& area = Areas[getAreaFromCharacter(c)];

		offset.X += area.underhang;
		if ( Invisible.findFirst ( c ) < 0 )
		{
			indices.push_back(area.spriteno);
			offsets.push_back(offset);
		}

		offset.X += area.width + area.overhang + GlobalKerningWidth;
	}

	SpriteBank->draw2DSpriteBatch(indices, offsets, clip, color);
}


//! Calculates the index of the character in the text which is on a specific position.
s32 CGUIFont::getCharacterFromPos(const wchar_t* text, s32 pixel_x) const
{
	s32 x = 0;
	s32 idx = 0;

	while (text[idx])
	{
		const SFontArea& a = Areas[getAreaFromCharacter(text[idx])];

		x += a.width + a.overhang + a.underhang + GlobalKerningWidth;

		if (x >= pixel_x)
			return idx;

		++idx;
	}

	return -1;
}


IGUISpriteBank* CGUIFont::getSpriteBank() const
{
	return SpriteBank;
}

} // end namespace gui
} // end namespace irr

#endif // _IRR_COMPILE_WITH_GUI_

