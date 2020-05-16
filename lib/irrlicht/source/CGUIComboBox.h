// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_GUI_COMBO_BOX_H_INCLUDED__
#define __C_GUI_COMBO_BOX_H_INCLUDED__

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_GUI_

#include "IGUIComboBox.h"
#include "IGUIStaticText.h"
#include "irrString.h"
#include "irrArray.h"

namespace irr
{
namespace gui
{
	class IGUIButton;
	class IGUIListBox;

	//! Single line edit box for editing simple text.
	class CGUIComboBox : public IGUIComboBox
	{
	public:

		//! constructor
		CGUIComboBox(IGUIEnvironment* environment, IGUIElement* parent,
			s32 id, core::rect<s32> rectangle);

		//! Returns amount of items in box
		virtual u32 getItemCount() const;

		//! returns string of an item. the idx may be a value from 0 to itemCount-1
		virtual const wchar_t* getItem(u32 idx) const;

		//! Returns item data of an item. the idx may be a value from 0 to itemCount-1
		virtual u32 getItemData(u32 idx) const;

		//! Returns index based on item data
		virtual s32 getIndexForItemData( u32 data ) const;

		//! adds an item and returns the index of it
		virtual u32 addItem(const wchar_t* text, u32 data);

		//! Removes an item from the combo box.
		virtual void removeItem(u32 id);

		//! deletes all items in the combo box
		virtual void clear();

		//! returns the text of the currently selected item
		virtual const wchar_t* getText() const;

		//! returns id of selected item. returns -1 if no item is selected.
		virtual s32 getSelected() const;

		//! sets the selected item. Set this to -1 if no item should be selected
		virtual void setSelected(s32 idx);

		//! sets the text alignment of the text part
		virtual void setTextAlignment(EGUI_ALIGNMENT horizontal, EGUI_ALIGNMENT vertical);

		//! Set the maximal number of rows for the selection listbox
		virtual void setMaxSelectionRows(u32 max);

		//! Get the maximimal number of rows for the selection listbox
		virtual u32 getMaxSelectionRows() const;

		//! called if an event happened.
		virtual bool OnEvent(const SEvent& event);

		//! draws the element and its children
		virtual void draw();

		//! Writes attributes of the element.
		virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const;

		//! Reads attributes of the element
		virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options);

	private:

		void openCloseMenu();
		void sendSelectionChangedEvent();

		IGUIButton* ListButton;
		IGUIStaticText* SelectedText;
		IGUIListBox* ListBox;
		IGUIElement *LastFocus;


		struct SComboData
		{
			SComboData ( const wchar_t * text, u32 data )
				: Name (text), Data ( data ) {}

			core::stringw Name;
			u32 Data;
		};
		core::array< SComboData > Items;

		s32 Selected;
		EGUI_ALIGNMENT HAlign, VAlign;
		u32 MaxSelectionRows;
		bool HasFocus;
	};


} // end namespace gui
} // end namespace irr

#endif // _IRR_COMPILE_WITH_GUI_

#endif // __C_GUI_COMBO_BOX_H_INCLUDED__

