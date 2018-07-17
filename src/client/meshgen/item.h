#pragma once
#include <vector>
#include "irrlichttypes.h"
#include <SMesh.h>

class Client;
struct ItemStack;

/*!
 * Holds color information of an item mesh's buffer.
 */
struct ItemPartColor
{
	/*!
	 * If this is false, the global base color of the item
	 * will be used instead of the specific color of the
	 * buffer.
	 */
	bool override_base = false;
	/*!
	 * The color of the buffer.
	 */
	video::SColor color = 0;

	ItemPartColor() = default;

	ItemPartColor(bool override, video::SColor color) :
			override_base(override), color(color)
	{
	}
};

struct ItemMesh
{
	scene::IMesh *mesh = nullptr;
	/*!
	 * Stores the color of each mesh buffer.
	 */
	std::vector<ItemPartColor> buffer_colors;
	/*!
	 * If false, all faces of the item should have the same brightness.
	 * Disables shading based on normal vectors.
	 */
	bool needs_shading = true;

	ItemMesh() = default;
};

ItemMesh createInventoryItemMesh(Client *client, const ItemStack &stack);
ItemMesh createWieldItemMesh(Client *client, const ItemStack &stack);
