#include "FormspecInventoryContext.h"

FormspecInventoryContext::~FormspecInventoryContext()
{
	if (m_selected_item != nullptr) {
		delete m_selected_item;
	}
}

ItemSpec *FormspecInventoryContext::getSelection()
{
	return m_selected_item;
}

bool FormspecInventoryContext::getSelectionIfValid(ItemSpec **out_selection)
{
	if (m_selected_item != nullptr && m_selected_item->isValid()) {
		*out_selection = m_selected_item;
		return true;
	}

	*out_selection = nullptr;
	return false;
}

bool FormspecInventoryContext::hasValidSelection()
{
	return m_selected_item != nullptr && m_selected_item->isValid();
}

void FormspecInventoryContext::setSelection(ItemSpec* selection)
{
	if (m_selected_item != nullptr) {
		delete m_selected_item;
	}

	m_selected_item = selection;

	// If we're clearing the selection, the selected amount is no longer valid
	if (!hasValidSelection ()) {
		m_selected_amount = 0;
	}
}

u16 FormspecInventoryContext::getSelectedAmount()
{
	return m_selected_amount;
}

void FormspecInventoryContext::setSelectedAmount(u16 amount)
{
	m_selected_amount = amount;
}

void FormspecInventoryContext::addHoveredItem(const ItemStack &item)
{
	m_hovered_items.push_back(item);
}

std::vector<ItemStack> FormspecInventoryContext::getHoveredItems()
{
	return m_hovered_items;
}

void FormspecInventoryContext::clearHoveredItems()
{
	m_hovered_items.clear();
}
