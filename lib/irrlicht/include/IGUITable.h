// Copyright (C) 2003-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_GUI_TABLE_H_INCLUDED__
#define __I_GUI_TABLE_H_INCLUDED__

#include "IGUIElement.h"
#include "irrTypes.h"
#include "SColor.h"
#include "IGUISkin.h"

namespace irr
{
namespace gui
{

	//! modes for ordering used when a column header is clicked
	enum EGUI_COLUMN_ORDERING
	{
		//! Do not use ordering
		EGCO_NONE,

		//! Send a EGET_TABLE_HEADER_CHANGED message when a column header is clicked.
		EGCO_CUSTOM,

		//! Sort it ascending by it's ascii value like: a,b,c,...
		EGCO_ASCENDING,

		//! Sort it descending by it's ascii value like: z,x,y,...
		EGCO_DESCENDING,

		//! Sort it ascending on first click, descending on next, etc
		EGCO_FLIP_ASCENDING_DESCENDING,

		//! Not used as mode, only to get maximum value for this enum
		EGCO_COUNT
	};

	//! Names for EGUI_COLUMN_ORDERING types
	const c8* const GUIColumnOrderingNames[] =
	{
		"none",
		"custom",
		"ascend",
		"descend",
		"ascend_descend",
		0,
	};

	enum EGUI_ORDERING_MODE
	{
		//! No element ordering
		EGOM_NONE,

		//! Elements are ordered from the smallest to the largest.
		EGOM_ASCENDING,

		//! Elements are ordered from the largest to the smallest.
		EGOM_DESCENDING,

		//! this value is not used, it only specifies the amount of default ordering types
		//! available.
		EGOM_COUNT
	};

	const c8* const GUIOrderingModeNames[] =
	{
		"none",
		"ascending",
		"descending",
		0
	};

	enum EGUI_TABLE_DRAW_FLAGS
	{
		EGTDF_ROWS = 1,
		EGTDF_COLUMNS = 2,
		EGTDF_ACTIVE_ROW = 4,
		EGTDF_COUNT
	};

	//! Default list box GUI element.
	/** \par This element can create the following events of type EGUI_EVENT_TYPE:
	\li EGET_TABLE_CHANGED
	\li EGET_TABLE_SELECTED_AGAIN
	\li EGET_TABLE_HEADER_CHANGED
	*/
	class IGUITable : public IGUIElement
	{
	public:
		//! constructor
		IGUITable(IGUIEnvironment* environment, IGUIElement* parent, s32 id, core::rect<s32> rectangle)
			: IGUIElement(EGUIET_TABLE, environment, parent, id, rectangle) {}

		//! Adds a column
		/** If columnIndex is outside the current range, do push new colum at the end */
		virtual void addColumn(const wchar_t* caption, s32 columnIndex=-1) = 0;

		//! remove a column from the table
		virtual void removeColumn(u32 columnIndex) = 0;

		//! Returns the number of columns in the table control
		virtual s32 getColumnCount() const = 0;

		//! Makes a column active. This will trigger an ordering process.
		/** \param idx: The id of the column to make active.
		\param doOrder: Do also the ordering which depending on mode for active column
		\return True if successful. */
		virtual bool setActiveColumn(s32 idx, bool doOrder=false) = 0;

		//! Returns which header is currently active
		virtual s32 getActiveColumn() const = 0;

		//! Returns the ordering used by the currently active column
		virtual EGUI_ORDERING_MODE getActiveColumnOrdering() const = 0;

		//! Set the width of a column
		virtual void setColumnWidth(u32 columnIndex, u32 width) = 0;

		//! Get the width of a column
		virtual u32 getColumnWidth(u32 columnIndex) const = 0;

		//! columns can be resized by drag 'n drop
		virtual void setResizableColumns(bool resizable) = 0;

		//! can columns be resized by dran 'n drop?
		virtual bool hasResizableColumns() const = 0;

		//! This tells the table control which ordering mode should be used when a column header is clicked.
		/** \param columnIndex The index of the column header.
		\param mode: One of the modes defined in EGUI_COLUMN_ORDERING */
		virtual void setColumnOrdering(u32 columnIndex, EGUI_COLUMN_ORDERING mode) = 0;

		//! Returns which row is currently selected
		virtual s32 getSelected() const = 0;

		//! set wich row is currently selected
		virtual void setSelected( s32 index ) = 0;

		//! Get amount of rows in the tabcontrol
		virtual s32 getRowCount() const = 0;

		//! adds a row to the table
		/** \param rowIndex Zero based index of rows. The row will be
		inserted at this position, if a row already exist there, it
		will be placed after it. If the row is larger than the actual
		number of row by more than one, it won't be created.  Note that
		if you create a row that's not at the end, there might be
		performance issues.
		\return index of inserted row. */
		virtual u32 addRow(u32 rowIndex) = 0;

		//! Remove a row from the table
		virtual void removeRow(u32 rowIndex) = 0;

		//! clears the table rows, but keeps the columns intact
		virtual void clearRows() = 0;

		//! Swap two row positions.
		virtual void swapRows(u32 rowIndexA, u32 rowIndexB) = 0;

		//! This tells the table to start ordering all the rows.
		/** You need to explicitly tell the table to re order the rows
		when a new row is added or the cells data is changed. This
		makes the system more flexible and doesn't make you pay the
		cost of ordering when adding a lot of rows.
		\param columnIndex: When set to -1 the active column is used.
		\param mode Ordering mode of the rows. */
		virtual void orderRows(s32 columnIndex=-1, EGUI_ORDERING_MODE mode=EGOM_NONE) = 0;

		//! Set the text of a cell
		virtual void setCellText(u32 rowIndex, u32 columnIndex, const core::stringw& text) = 0;

		//! Set the text of a cell, and set a color of this cell.
		virtual void setCellText(u32 rowIndex, u32 columnIndex, const core::stringw& text, video::SColor color) = 0;

		//! Set the data of a cell
		virtual void setCellData(u32 rowIndex, u32 columnIndex, void *data) = 0;

		//! Set the color of a cell text
		virtual void setCellColor(u32 rowIndex, u32 columnIndex, video::SColor color) = 0;

		//! Get the text of a cell
		virtual const wchar_t* getCellText(u32 rowIndex, u32 columnIndex ) const = 0;

		//! Get the data of a cell
		virtual void* getCellData(u32 rowIndex, u32 columnIndex ) const = 0;

		//! clears the table, deletes all items in the table
		virtual void clear() = 0;

		//! Set flags, as defined in EGUI_TABLE_DRAW_FLAGS, which influence the layout
		virtual void setDrawFlags(s32 flags) = 0;

		//! Get the flags, as defined in EGUI_TABLE_DRAW_FLAGS, which influence the layout
		virtual s32 getDrawFlags() const = 0;
	};


} // end namespace gui
} // end namespace irr

#endif

