// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __E_GUI_ELEMENT_TYPES_H_INCLUDED__
#define __E_GUI_ELEMENT_TYPES_H_INCLUDED__

#include "irrTypes.h"

namespace irr
{
namespace gui
{

//! List of all basic Irrlicht GUI elements.
/** An IGUIElement returns this when calling IGUIElement::getType(); */
enum EGUI_ELEMENT_TYPE
{
	//! A button (IGUIButton)
	EGUIET_BUTTON = 0,

	//! A check box (IGUICheckBox)
	EGUIET_CHECK_BOX,

	//! A combo box (IGUIComboBox)
	EGUIET_COMBO_BOX,

	//! A context menu (IGUIContextMenu)
	EGUIET_CONTEXT_MENU,

	//! A menu (IGUIMenu)
	EGUIET_MENU,

	//! An edit box (IGUIEditBox)
	EGUIET_EDIT_BOX,

	//! A file open dialog (IGUIFileOpenDialog)
	EGUIET_FILE_OPEN_DIALOG,

	//! A color select open dialog (IGUIColorSelectDialog)
	EGUIET_COLOR_SELECT_DIALOG,

	//! A in/out fader (IGUIInOutFader)
	EGUIET_IN_OUT_FADER,

	//! An image (IGUIImage)
	EGUIET_IMAGE,

	//! A list box (IGUIListBox)
	EGUIET_LIST_BOX,

	//! A mesh viewer (IGUIMeshViewer)
	EGUIET_MESH_VIEWER,

	//! A message box (IGUIWindow)
	EGUIET_MESSAGE_BOX,

	//! A modal screen
	EGUIET_MODAL_SCREEN,

	//! A scroll bar (IGUIScrollBar)
	EGUIET_SCROLL_BAR,

	//! A spin box (IGUISpinBox)
	EGUIET_SPIN_BOX,

	//! A static text (IGUIStaticText)
	EGUIET_STATIC_TEXT,

	//! A tab (IGUITab)
	EGUIET_TAB,

	//! A tab control
	EGUIET_TAB_CONTROL,

	//! A Table
	EGUIET_TABLE,

	//! A tool bar (IGUIToolBar)
	EGUIET_TOOL_BAR,

	//! A Tree View
	EGUIET_TREE_VIEW,

	//! A window
	EGUIET_WINDOW,

	//! Unknown type.
	EGUIET_ELEMENT,

	//! The root of the GUI
	EGUIET_ROOT,

	//! Not an element, amount of elements in there
	EGUIET_COUNT,

	//! This enum is never used, it only forces the compiler to compile this enumeration to 32 bit.
	EGUIET_FORCE_32_BIT = 0x7fffffff

};

//! Names for built-in element types
const c8* const GUIElementTypeNames[] =
{
	"button",
	"checkBox",
	"comboBox",
	"contextMenu",
	"menu",
	"editBox",
	"fileOpenDialog",
	"colorSelectDialog",
	"inOutFader",
	"image",
	"listBox",
	"meshViewer",
	"messageBox",
	"modalScreen",
	"scrollBar",
	"spinBox",
	"staticText",
	"tab",
	"tabControl",
	"table",
	"toolBar",
	"treeview",
	"window",
	"element",
	"root",
	0
};

} // end namespace gui
} // end namespace irr

#endif




