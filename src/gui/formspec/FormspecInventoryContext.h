#pragma once
#include "ItemSpec.h"

class FormspecInventoryContext
{
public:
	void addHoveredItem(const ItemStack &item);
    std::vector<ItemStack> getHoveredItems();
    void clearHoveredItems();
private:
    std::vector<ItemStack> m_hovered_items;
};
