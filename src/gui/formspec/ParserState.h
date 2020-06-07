#pragma once

#include <IGUIElement.h>
#include <stack>

#include "../guiTable.h"
#include "FieldSpec.h"

class ParserState {
	irr::gui::IGUIElement *parent;
	std::vector<FieldSpec> specs;

public:
	irr::gui::IGUIEnvironment *Environment;

	const InventoryLocation inventoryLocation;
	int formspec_version = 1;
	bool real_coordinates = false;
	bool explicit_size = false;
	v2f32 pos_offset{};

	std::stack<v2f32> container_stack{};
	v2s32 imgsize{};
	v2s32 padding{};
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

	v2s32 getElementBasePos(const v2f32 &pos) const
	{
		v2f32 pos_f = v2f32(padding.X, padding.Y) + pos_offset * spacing;
		pos_f.X += pos.X * spacing.X;
		pos_f.Y += pos.Y * spacing.Y;
		return v2s32(pos_f.X, pos_f.Y);
	}

	v2s32 getRealCoordinateBasePos(const v2f32 &pos) const
	{
		return v2s32((pos.X + pos_offset.X) * imgsize.X,
					 (pos.Y + pos_offset.Y) * imgsize.Y);
	}

	v2s32 getRealCoordinateGeometry(const v2f32 &geom) const
	{
		return v2s32(geom.X * imgsize.X, geom.Y * imgsize.Y);
	}

	v2s32 getPosition(v2f32 pos) const
	{
		return real_coordinates ? getRealCoordinateBasePos(pos)
							   : getElementBasePos(pos);
	}

	FieldSpec &makeSpec(int priority) {
		specs.emplace_back("", L"", L"", 258 + specs.size(), priority);
		return specs[specs.size() - 1];
	}

	v2s32 getElementBasePosOld(const std::vector<std::string> &v_pos)
	{
		return getElementBasePos(v2f32(stof(v_pos[0]), stof(v_pos[1])));
	}

	v2s32 getRealCoordinateBasePosOld(const std::vector<std::string> &v_pos)
	{
		return getRealCoordinateBasePos(v2f32(stof(v_pos[0]), stof(v_pos[1])));
	}

	v2s32 getRealCoordinateGeometryOld(const std::vector<std::string> &v_geom)
	{
		return getRealCoordinateGeometry(v2f32(stof(v_geom[0]), stof(v_geom[1])));
	}
};
