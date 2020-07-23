#pragma once
#include "ItemSpec.h"

class FormspecInventoryContext
{
public:
	~FormspecInventoryContext();

	ItemSpec *getSelection();
	bool getSelectionIfValid(ItemSpec **out_selection);
	bool hasValidSelection();
	void setSelection(ItemSpec *selection);

	u16 getSelectedAmount();
	void setSelectedAmount(u16 amount);

	void addHoveredItem(const ItemStack &item);
	std::vector<ItemStack> getHoveredItems();
	void clearHoveredItems();

private:
	std::vector<ItemStack> m_hovered_items;
	u16 m_selected_amount = 0;
	ItemSpec *m_selected_item = nullptr;
};
