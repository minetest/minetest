/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#ifndef GUIINVENTORYMENU_HEADER
#define GUIINVENTORYMENU_HEADER

#include "irrlichttypes_extrabloated.h"
#include "inventory.h"
#include "inventorymanager.h"
#include "modalMenu.h"
#include "client.h"
#include "clientmap.h"
#include "util/string.h"
#include "util/numeric.h"
#include "mainmenumanager.h"
#include "mapblock.h"
#include "nodedef.h"
#include "nodemetadata.h"

#include "gettext.h"

class IGameDef;
class InventoryManager;

struct TextDest
{
	virtual void gotText(std::wstring text) = 0;
	virtual void gotText(std::map<std::string, std::string> fields) = 0;
	virtual ~TextDest() {};
};

struct TextDestChat : public TextDest
{
	TextDestChat(Client *client)
	{
		m_client = client;
	}
	void gotText(std::wstring text)
	{
		m_client->typeChatMessage(text);
	}
	void gotText(std::map<std::string, std::string> fields)
	{
		m_client->typeChatMessage(narrow_to_wide(fields["text"]));
	}

	Client *m_client;
};

struct TextDestNodeMetadata : public TextDest
{
	TextDestNodeMetadata(v3s16 p, Client *client)
	{
		m_p = p;
		m_client = client;
	}
	void gotText(std::wstring text)
	{
		std::string ntext = wide_to_narrow(text);
		infostream<<"Changing text of a sign node: "
				<<ntext<<std::endl;
		std::map<std::string, std::string> fields;
		fields["text"] = ntext;
		m_client->sendNodemetaFields(m_p, "", fields);
	}
	void gotText(std::map<std::string, std::string> fields)
	{
		m_client->sendNodemetaFields(m_p, "", fields);
	}

	v3s16 m_p;
	Client *m_client;
};

class IFormSource
{
public:
	virtual ~IFormSource(){}
	virtual std::string getForm() = 0;
	v3s16 m_p;
};

/* Form update callback */

class NodeMetadataFormSource: public IFormSource
{
public:
	NodeMetadataFormSource(ClientMap *map, v3s16 p):
		m_map(map),
		m_p(p)
	{
	}
	std::string getForm()
	{
		NodeMetadata *meta = m_map->getNodeMetadata(m_p);
		if(!meta)
			return "";
		return meta->getString("formspec");
	}

	ClientMap *m_map;
	v3s16 m_p;
};

void drawItemStack(video::IVideoDriver *driver,
		gui::IGUIFont *font,
		const ItemStack &item,
		const core::rect<s32> &rect,
		const core::rect<s32> *clip,
		IGameDef *gamedef);

class GUIFormSpecMenu : public GUIModalMenu
{
	struct ItemSpec
	{
		ItemSpec()
		{
			i = -1;
		}
		ItemSpec(const InventoryLocation &a_inventoryloc,
				const std::string &a_listname,
				s32 a_i)
		{
			inventoryloc = a_inventoryloc;
			listname = a_listname;
			i = a_i;
		}
		bool isValid() const
		{
			return i != -1;
		}

		InventoryLocation inventoryloc;
		std::string listname;
		s32 i;
	};

	struct ListDrawSpec
	{
		ListDrawSpec()
		{
		}
		ListDrawSpec(const InventoryLocation &a_inventoryloc,
				const std::string &a_listname,
				v2s32 a_pos, v2s32 a_geom):
			inventoryloc(a_inventoryloc),
			listname(a_listname),
			pos(a_pos),
			geom(a_geom)
		{
		}

		InventoryLocation inventoryloc;
		std::string listname;
		v2s32 pos;
		v2s32 geom;
	};

	struct ImageDrawSpec
	{
		ImageDrawSpec()
		{
		}
		ImageDrawSpec(const std::string &a_name,
				v2s32 a_pos, v2s32 a_geom):
			name(a_name),
			pos(a_pos),
			geom(a_geom)
		{
		}
		std::string name;
		v2s32 pos;
		v2s32 geom;
	};
	
	struct FieldSpec
	{
		FieldSpec()
		{
		}
		FieldSpec(const std::wstring name, const std::wstring label, const std::wstring fdeflt, int id):
			fname(name),
			flabel(label),
			fdefault(fdeflt),
			fid(id)
		{
			send = false;
			is_button = false;
		}
		std::wstring fname;
		std::wstring flabel;
		std::wstring fdefault;
		int fid;
		bool send;
		bool is_button;
	};

public:
	GUIFormSpecMenu(gui::IGUIEnvironment* env,
			gui::IGUIElement* parent, s32 id,
			IMenuManager *menumgr,
			InventoryManager *invmgr,
			IGameDef *gamedef
			);
	~GUIFormSpecMenu();

	void setFormSpec(const std::string &formspec_string,
			InventoryLocation current_inventory_location)
	{
		m_formspec_string = formspec_string;
		m_current_inventory_location = current_inventory_location;
		regenerateGui(m_screensize_old);
	}
	
	// form_src is deleted by this GUIFormSpecMenu
	void setFormSource(IFormSource *form_src)
	{
		m_form_src = form_src;
		m_dest = new TextDestNodeMetadata(((NodeMetadataFormSource*)form_src)->m_p,(Client*)m_gamedef);
	}

	void removeChildren();
	/*
		Remove and re-add (or reposition) stuff
	*/
	void regenerateGui(v2u32 screensize);
	
	ItemSpec getItemAtPos(v2s32 p) const;
	void drawList(const ListDrawSpec &s, int phase);
	void drawSelectedItem();
	void drawMenu();
	void updateSelectedItem();

	void acceptInput();
	bool OnEvent(const SEvent& event);
	
protected:
	v2s32 getBasePos() const
	{
		return padding + AbsoluteRect.UpperLeftCorner;
	}

	v2s32 padding;
	v2s32 spacing;
	v2s32 imgsize;
	
	InventoryManager *m_invmgr;
	IGameDef *m_gamedef;

	std::string m_formspec_string;
	InventoryLocation m_current_inventory_location;
	IFormSource *m_form_src;

	core::array<ListDrawSpec> m_inventorylists;
	core::array<ImageDrawSpec> m_images;
	core::array<FieldSpec> m_fields;

	ItemSpec *m_selected_item;
	u32 m_selected_amount;
	bool m_selected_dragging;

	v2s32 m_pointer;
	gui::IGUIStaticText *m_tooltip_element;
private:
	TextDest *m_dest;
};

#endif

