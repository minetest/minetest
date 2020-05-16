// This file is part of the "Irrlicht Engine".
// written by Reinhard Ostermeier, reinhard@nospam.r-ostermeier.de

#ifndef __C_GUI_IMAGE_LIST_H_INCLUDED__
#define __C_GUI_IMAGE_LIST_H_INCLUDED__

#include "IGUIImageList.h"
#include "IVideoDriver.h"

namespace irr
{
namespace gui
{

class CGUIImageList : public IGUIImageList
{
public:

	//! constructor
	CGUIImageList( video::IVideoDriver* Driver );

	//! destructor
	virtual ~CGUIImageList();

	//! Creates the image list from texture.
	//! \param texture: The texture to use
	//! \param imageSize: Size of a single image
	//! \param useAlphaChannel: true if the alpha channel from the texture should be used
	//! \return
	//! true if the image list was created
	bool createImageList( 
				video::ITexture*			texture, 
				core::dimension2d<s32>	imageSize, 
				bool							useAlphaChannel );

	//! Draws an image and clips it to the specified rectangle if wanted
	//! \param index: Index of the image
	//! \param destPos: Position of the image to draw
	//! \param clip: Optional pointer to a rectalgle against which the text will be clipped.
	//! If the pointer is null, no clipping will be done.
	virtual void draw( s32 index, const core::position2d<s32>& destPos, 
		const core::rect<s32>* clip = 0 );

	//! Returns the count of Images in the list.
	//! \return Returns the count of Images in the list.
	virtual s32 getImageCount() const
	{ return ImageCount; }

	//! Returns the size of the images in the list.
	//! \return Returns the size of the images in the list.
	virtual core::dimension2d<s32> getImageSize() const
	{ return ImageSize; }

private:

	video::IVideoDriver*		Driver;
	video::ITexture*			Texture;
	s32							ImageCount;
	core::dimension2d<s32>	ImageSize;
	s32							ImagesPerRow;
	bool							UseAlphaChannel;
};

} // end namespace gui
} // end namespace irr

#endif

