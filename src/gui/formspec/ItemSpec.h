#pragma once
#include "inventorymanager.h"
#include "irrlichttypes_extrabloated.h"
#include "util/string.h"

struct ItemSpec
{
	ItemSpec() = default;

	ItemSpec(const InventoryLocation &a_inventoryloc, const std::string &a_listname,
			s32 a_i) :
			inventoryloc(a_inventoryloc),
			listname(a_listname), i(a_i)
	{
	}

	bool isValid() const { return i != -1; }

	InventoryLocation inventoryloc;
	std::string listname;
	s32 i = -1;
};
