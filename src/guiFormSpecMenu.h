/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include <utility>

#include "irrlichttypes_extrabloated.h"
#include "inventory.h"
#include "inventorymanager.h"
#include "modalMenu.h"
#include "guiTable.h"
#include "clientserver.h"

class IGameDef;
class InventoryManager;
class ISimpleTextureSource;
class Client;

typedef enum {
	f_Button,
	f_Table,
	f_TabHeader,
	f_CheckBox,
	f_DropDown,
	f_ScrollBar,
	f_Unknown
} FormspecFieldType;

typedef enum {
	quit_mode_no,
	quit_mode_accept,
	quit_mode_cancel
} FormspecQuitMode;

struct TextDest
{
	virtual ~TextDest() {};
	// This is deprecated I guess? -celeron55
	virtual void gotText(std::wstring text){}
	virtual void gotText(std::map<std::string, std::string> fields) = 0;
	virtual void setFormName(std::string formname)
	{ m_formname = formname;};

	std::string m_formname;
};

class IFormSource
{
public:
	virtual ~IFormSource(){}
	virtual std::string getForm() = 0;
	// Fill in variables in field text
	virtual std::string resolveText(std::string str){ return str; }
};

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
				v2s32 a_pos, v2s32 a_geom, s32 a_start_item_i):
			inventoryloc(a_inventoryloc),
			listname(a_listname),
			pos(a_pos),
			geom(a_geom),
			start_item_i(a_start_item_i)
		{
		}

		InventoryLocation inventoryloc;
		std::string listname;
		v2s32 pos;
		v2s32 geom;
		s32 start_item_i;
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
			scale = true;
		}
		ImageDrawSpec(const std::string &a_name,
				v2s32 a_pos):
			name(a_name),
			pos(a_pos)
		{
			scale = false;
		}
		std::string name;
		v2s32 pos;
		v2s32 geom;
		bool scale;
	};

	struct FieldSpec
	{
		FieldSpec()
		{
		}
		FieldSpec(const std::wstring &name, const std::wstring &label,
				const std::wstring &fdeflt, int id) :
			fname(name),
			flabel(label),
			fdefault(fdeflt),
			fid(id)
		{
			send = false;
			ftype = f_Unknown;
			is_exit = false;
		}
		std::wstring fname;
		std::wstring flabel;
		std::wstring fdefault;
		int fid;
		bool send;
		FormspecFieldType ftype;
		bool is_exit;
		core::rect<s32> rect;
	};

	struct BoxDrawSpec {
		BoxDrawSpec(v2s32 a_pos, v2s32 a_geom,irr::video::SColor a_color):
			pos(a_pos),
			geom(a_geom),
			color(a_color)
		{
		}
		v2s32 pos;
		v2s32 geom;
		irr::video::SColor color;
	};

	struct TooltipSpec {
		TooltipSpec()
		{
		}
		TooltipSpec(std::string a_tooltip, irr::video::SColor a_bgcolor,
				irr::video::SColor a_color):
			tooltip(a_tooltip),
			bgcolor(a_bgcolor),
			color(a_color)
		{
		}
		std::string tooltip;
		irr::video::SColor bgcolor;
		irr::video::SColor color;
	};

public:
	GUIFormSpecMenu(irr::IrrlichtDevice* dev,
			gui::IGUIElement* parent, s32 id,
			IMenuManager *menumgr,
			InventoryManager *invmgr,
			IGameDef *gamedef,
			ISimpleTextureSource *tsrc,
			IFormSource* fs_src,
			TextDest* txt_dst,
			Client* client
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
		if (m_form_src != NULL) {
			delete m_form_src;
		}
		m_form_src = form_src;
	}

	// text_dst is deleted by this GUIFormSpecMenu
	void setTextDest(TextDest *text_dst)
	{
		if (m_text_dst != NULL) {
			delete m_text_dst;
		}
		m_text_dst = text_dst;
	}

	void allowClose(bool value)
	{
		m_allowclose = value;
	}

	void lockSize(bool lock,v2u32 basescreensize=v2u32(0,0)) {
		m_lock = lock;
		m_lockscreensize = basescreensize;
	}

	void removeChildren();
	void setInitialFocus();
	/*
		Remove and re-add (or reposition) stuff
	*/
	void regenerateGui(v2u32 screensize);

	ItemSpec getItemAtPos(v2s32 p) const;
	void drawList(const ListDrawSpec &s, int phase);
	void drawSelectedItem();
	void drawMenu();
	void updateSelectedItem();
	ItemStack verifySelectedItem();

	void acceptInput(FormspecQuitMode quitmode);
	bool preprocessEvent(const SEvent& event);
	bool OnEvent(const SEvent& event);
	bool doPause;
	bool pausesGame() { return doPause; }

	GUITable* getTable(std::wstring tablename);

#ifdef __ANDROID__
	bool getAndroidUIInput();
#endif

protected:
	v2s32 getBasePos() const
	{
			return padding + offset + AbsoluteRect.UpperLeftCorner;
	}

	v2s32 padding;
	v2s32 spacing;
	v2s32 imgsize;
	v2s32 offset;

	irr::IrrlichtDevice* m_device;
	InventoryManager *m_invmgr;
	IGameDef *m_gamedef;
	ISimpleTextureSource *m_tsrc;
	Client *m_client;

	std::string m_formspec_string;
	InventoryLocation m_current_inventory_location;


	std::vector<ListDrawSpec> m_inventorylists;
	std::vector<ImageDrawSpec> m_backgrounds;
	std::vector<ImageDrawSpec> m_images;
	std::vector<ImageDrawSpec> m_itemimages;
	std::vector<BoxDrawSpec> m_boxes;
	std::vector<FieldSpec> m_fields;
	std::vector<std::pair<FieldSpec,GUITable*> > m_tables;
	std::vector<std::pair<FieldSpec,gui::IGUICheckBox*> > m_checkboxes;
	std::map<std::wstring, TooltipSpec> m_tooltips;
	std::vector<std::pair<FieldSpec,gui::IGUIScrollBar*> > m_scrollbars;

	ItemSpec *m_selected_item;
	u32 m_selected_amount;
	bool m_selected_dragging;

	// WARNING: BLACK MAGIC
	// Used to guess and keep up with some special things the server can do.
	// If name is "", no guess exists.
	ItemStack m_selected_content_guess;
	InventoryLocation m_selected_content_guess_inventory;

	v2s32 m_pointer;
	v2s32 m_old_pointer;  // Mouse position after previous mouse event
	gui::IGUIStaticText *m_tooltip_element;

	u32 m_tooltip_show_delay;
	s32 m_hoovered_time;
	s32 m_old_tooltip_id;
	std::string m_old_tooltip;

	bool m_allowclose;
	bool m_lock;
	v2u32 m_lockscreensize;

	bool m_bgfullscreen;
	bool m_slotborder;
	bool m_clipbackground;
	video::SColor m_bgcolor;
	video::SColor m_slotbg_n;
	video::SColor m_slotbg_h;
	video::SColor m_slotbordercolor;
	video::SColor m_default_tooltip_bgcolor;
	video::SColor m_default_tooltip_color;

private:
	IFormSource      *m_form_src;
	TextDest         *m_text_dst;
	gui::IGUIFont    *m_font;
	unsigned int      m_formspec_version;

	typedef struct {
		v2s32 size;
		core::rect<s32> rect;
		v2s32 basepos;
		int bp_set;
		v2u32 screensize;
		std::wstring focused_fieldname;
		GUITable::TableOptions table_options;
		GUITable::TableColumns table_columns;
		// used to restore table selection/scroll/treeview state
		std::map<std::wstring,GUITable::DynamicData> table_dyndata;
	} parserData;

	typedef struct {
		bool key_up;
		bool key_down;
		bool key_enter;
		bool key_escape;
	} fs_key_pendig;

	fs_key_pendig current_keys_pending;

	void parseElement(parserData* data,std::string element);

	void parseSize(parserData* data,std::string element);
	void parseList(parserData* data,std::string element);
	void parseCheckbox(parserData* data,std::string element);
	void parseImage(parserData* data,std::string element);
	void parseItemImage(parserData* data,std::string element);
	void parseButton(parserData* data,std::string element,std::string typ);
	void parseBackground(parserData* data,std::string element);
	void parseTableOptions(parserData* data,std::string element);
	void parseTableColumns(parserData* data,std::string element);
	void parseTable(parserData* data,std::string element);
	void parseTextList(parserData* data,std::string element);
	void parseDropDown(parserData* data,std::string element);
	void parsePwdField(parserData* data,std::string element);
	void parseField(parserData* data,std::string element,std::string type);
	void parseSimpleField(parserData* data,std::vector<std::string> &parts);
	void parseTextArea(parserData* data,std::vector<std::string>& parts,
			std::string type);
	void parseLabel(parserData* data,std::string element);
	void parseVertLabel(parserData* data,std::string element);
	void parseImageButton(parserData* data,std::string element,std::string type);
	void parseItemImageButton(parserData* data,std::string element);
	void parseTabHeader(parserData* data,std::string element);
	void parseBox(parserData* data,std::string element);
	void parseBackgroundColor(parserData* data,std::string element);
	void parseListColors(parserData* data,std::string element);
	void parseTooltip(parserData* data,std::string element);
	bool parseVersionDirect(std::string data);
	void parseScrollBar(parserData* data, std::string element);

	/**
	 * check if event is part of a double click
	 * @param event event to evaluate
	 * @return true/false if a doubleclick was detected
	 */
	bool DoubleClickDetection(const SEvent event);

	struct clickpos
	{
		v2s32 pos;
		s32 time;
	};
	clickpos m_doubleclickdetect[2];

	int m_btn_height;

	std::wstring getLabelByID(s32 id);
	std::wstring getNameByID(s32 id);
#ifdef __ANDROID__
	v2s32 m_down_pos;
	std::wstring m_JavaDialogFieldName;
#endif

};

class FormspecFormSource: public IFormSource
{
public:
	FormspecFormSource(std::string formspec)
	{
		m_formspec = formspec;
	}

	~FormspecFormSource()
	{}

	void setForm(std::string formspec) {
		m_formspec = FORMSPEC_VERSION_STRING + formspec;
	}

	std::string getForm()
	{
		return m_formspec;
	}

	std::string m_formspec;
};

#endif

