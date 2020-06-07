#pragma once

#include <IGUIElement.h>
#include <stack>

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

	GUIInventoryList::Options inventoryListOptions;

	ParserState(irr::gui::IGUIEnvironment *Environment,
				irr::gui::IGUIElement *parent,
				InventoryLocation &inventoryLocation):
			parent(parent), Environment(Environment),
			inventoryLocation(inventoryLocation)
	{}

	irr::gui::IGUIElement *getParentElement() const {
		return parent;
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
