// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_GUI_FILE_OPEN_DIALOG_H_INCLUDED__
#define __I_GUI_FILE_OPEN_DIALOG_H_INCLUDED__

#include "IGUIElement.h"
#include "path.h"

namespace irr
{
namespace gui
{

	//! Standard file chooser dialog.
	/** \warning When the user selects a folder this does change the current working directory

	\par This element can create the following events of type EGUI_EVENT_TYPE:
	\li EGET_DIRECTORY_SELECTED
	\li EGET_FILE_SELECTED
	\li EGET_FILE_CHOOSE_DIALOG_CANCELLED
	*/
	class IGUIFileOpenDialog : public IGUIElement
	{
	public:

		//! constructor
		IGUIFileOpenDialog(IGUIEnvironment* environment, IGUIElement* parent, s32 id, core::rect<s32> rectangle)
			: IGUIElement(EGUIET_FILE_OPEN_DIALOG, environment, parent, id, rectangle) {}

		//! Returns the filename of the selected file. Returns NULL, if no file was selected.
		virtual const wchar_t* getFileName() const = 0;

		//! Returns the directory of the selected file. Returns NULL, if no directory was selected.
		virtual const io::path& getDirectoryName() = 0;
	};


} // end namespace gui
} // end namespace irr

#endif

