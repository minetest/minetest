#pragma once

#include <IGUIElement.h>
#include <stack>

#include "../guiTable.h"

class ParserState {
	irr::gui::IGUIElement *parent;

public:
	irr::gui::IGUIEnvironment *Environment;

	const InventoryLocation inventoryLocation;
	int formspec_version = 1;
	bool real_coordinates = false;
	bool explicit_size = false;
	v2f32 pos_offset{};

	std::stack<v2f32> container_stack{};
	v2f32 imgsize{};
	v2f32 padding{};
	v2f32 spacing{};

	u8 simple_field_count;
	v2f invsize;
	v2s32 size;
	v2f32 offset;
	v2f32 anchor;
	core::rect<s32> rect;
	v2s32 basepos;
	v2u32 screensize;
	std::string focused_fieldname;
	GUITable::TableOptions table_options;
	GUITable::TableColumns table_columns;
	GUIInventoryList::Options inventorylist_options;

	struct {
		s32 max = 1000;
		s32 min = 0;
		s32 small_step = 10;
		s32 large_step = 100;
		s32 thumb_size = 1;
		GUIScrollBar::ArrowVisibility arrow_visiblity = GUIScrollBar::DEFAULT;
	} scrollbar_options;

	// used to restore table selection/scroll/treeview state
	std::unordered_map<std::string, GUITable::DynamicData> table_dyndata;

	ParserState(irr::gui::IGUIEnvironment *Environment,
				irr::gui::IGUIElement *parent,
				InventoryLocation &inventoryLocation):
			parent(parent), Environment(Environment),
			inventoryLocation(inventoryLocation)
	{}

	ParserState(const ParserState &other) = delete;

	irr::gui::IGUIElement *getParentElement() const {
		return parent;
	}

	void setParentElement(irr::gui::IGUIElement *v) {
		parent = v;
	}

	v2s32 getElementBasePos(bool hasPos, const v2f32 &pos)
	{
		v2f32 pos_f = v2f32(padding.X, padding.Y) + pos_offset * spacing;
		if (hasPos) {
			pos_f.X += pos.X * spacing.X;
			pos_f.Y += pos.Y * spacing.Y;
		}
		return v2s32(pos_f.X, pos_f.Y);
	}

	v2s32 getRealCoordinateBasePos(const v2f32 &pos)
	{
		return v2s32((pos.X + pos_offset.X) * imgsize.X,
					 (pos.Y + pos_offset.Y) * imgsize.Y);
	}

	v2s32 getRealCoordinateGeometry(const v2f32 &geom)
	{
		return v2s32(geom.X * imgsize.X, geom.Y * imgsize.Y);
	}
};
