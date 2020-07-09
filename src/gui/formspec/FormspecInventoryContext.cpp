#include "FormspecInventoryContext.h"

void FormspecInventoryContext::addHoveredItem(const ItemStack &item)
{
    m_hovered_items.push_back(item);
}

std::vector<ItemStack> FormspecInventoryContext::getHoveredItems() {
    return m_hovered_items;
}

void FormspecInventoryContext::clearHoveredItems() {
    m_hovered_items.clear();
}
