// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_GUI_MESSAGE_BOX_H_INCLUDED__
#define __C_GUI_MESSAGE_BOX_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_GUI_

#include "CGUIWindow.h"
#include "IGUIStaticText.h"
#include "IGUIImage.h"
#include "irrArray.h"

namespace irr
{
namespace gui
{
	class CGUIMessageBox : public CGUIWindow
	{
	public:

		//! constructor
		CGUIMessageBox(IGUIEnvironment* environment, const wchar_t* caption,
			const wchar_t* text, s32 flag,
			IGUIElement* parent, s32 id, core::rect<s32> rectangle, video::ITexture* image=0);

		//! destructor
		virtual ~CGUIMessageBox();

		//! called if an event happened.
		virtual bool OnEvent(const SEvent& event);

		//! Writes attributes of the element.
		virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;

		//! Reads attributes of the element
		virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);

	private:

		void refreshControls();
		void setButton(IGUIButton*& button, bool isAvailable, const core::rect<s32> & btnRect, const wchar_t * text, IGUIElement*& focusMe);

		IGUIButton* OkButton;
		IGUIButton* CancelButton;
		IGUIButton* YesButton;
		IGUIButton* NoButton;
		IGUIStaticText* StaticText;
		IGUIImage * Icon;
		video::ITexture * IconTexture;

		s32 Flags;
		core::stringw MessageText;
		bool Pressed;
	};

} // end namespace gui
} // end namespace irr

#endif // _IRR_COMPILE_WITH_GUI_

#endif
