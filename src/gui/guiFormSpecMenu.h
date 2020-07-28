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

#pragma once

#include <utility>
#include <stack>
#include <unordered_set>

#include "irrlichttypes_extrabloated.h"
#include "inventorymanager.h"
#include "modalMenu.h"
#include "guiInventoryList.h"
#include "guiScrollBar.h"
#include "guiTable.h"
#include "network/networkprotocol.h"
#include "client/joystick_controller.h"
#include "util/string.h"
#include "util/enriched_string.h"
#include "StyleSpec.h"

class InventoryManager;
class ISimpleTextureSource;
class Client;
class TexturePool;
class GUIScrollContainer;

typedef enum {
	f_Button,
	f_Table,
	f_TabHeader,
	f_CheckBox,
	f_DropDown,
	f_ScrollBar,
	f_Box,
	f_ItemImage,
	f_HyperText,
	f_AnimatedImage,
	f_Unknown
} FormspecFieldType;

typedef enum {
	quit_mode_no,
	quit_mode_accept,
	quit_mode_cancel
} FormspecQuitMode;

struct TextDest
{
	virtual ~TextDest() = default;

	// This is deprecated I guess? -celeron55
	virtual void gotText(const std::wstring &text) {}
	virtual void gotText(const StringMap &fields) = 0;

	std::string m_formname;
};

class IFormSource
{
public:
	virtual ~IFormSource() = default;
	virtual const std::string &getForm() const = 0;
	// Fill in variables in field text
	virtual std::string resolveText(const std::string &str) { return str; }
};

class GUIFormSpecMenu : public GUIModalMenu
{
	struct ListRingSpec
	{
		ListRingSpec() = default;

		ListRingSpec(const InventoryLocation &a_inventoryloc,
				const std::string &a_listname):
			inventoryloc(a_inventoryloc),
			listname(a_listname)
		{
		}

		InventoryLocation inventoryloc;
		std::string listname;
	};

	struct FieldSpec
	{
		FieldSpec() = default;

		FieldSpec(const std::string &name, const std::wstring &label,
				const std::wstring &default_text, s32 id, int priority = 0,
				gui::ECURSOR_ICON cursor_icon = ECI_NORMAL) :
			fname(name),
			flabel(label),
			fdefault(unescape_enriched(translate_string(default_text))),
			fid(id),
			send(false),
			ftype(f_Unknown),
			is_exit(false),
			priority(priority),
			fcursor_icon(cursor_icon)
		{
		}

		std::string fname;
		std::wstring flabel;
		std::wstring fdefault;
		s32 fid;
		bool send;
		FormspecFieldType ftype;
		bool is_exit;
		// Draw priority for formspec version < 3
		int priority;
		core::rect<s32> rect;
		gui::ECURSOR_ICON fcursor_icon;
	};

	struct TooltipSpec
	{
		TooltipSpec() = default;
		TooltipSpec(const std::wstring &a_tooltip, irr::video::SColor a_bgcolor,
				irr::video::SColor a_color):
			tooltip(translate_string(a_tooltip)),
			bgcolor(a_bgcolor),
			color(a_color)
		{
		}

		std::wstring tooltip;
		irr::video::SColor bgcolor;
		irr::video::SColor color;
	};

public:
	GUIFormSpecMenu(JoystickController *joystick,
			gui::IGUIElement* parent, s32 id,
			IMenuManager *menumgr,
			Client *client,
			ISimpleTextureSource *tsrc,
			IFormSource* fs_src,
			TextDest* txt_dst,
			const std::string &formspecPrepend,
			bool remap_dbl_click = true);

	~GUIFormSpecMenu();

	static void create(GUIFormSpecMenu *&cur_formspec, Client *client,
		JoystickController *joystick, IFormSource *fs_src, TextDest *txt_dest,
		const std::string &formspecPrepend);

	void setFormSpec(const std::string &formspec_string,
			const InventoryLocation &current_inventory_location)
	{
		m_formspec_string = formspec_string;
		m_current_inventory_location = current_inventory_location;
		m_is_form_regenerated = false;
		regenerateGui(m_screensize_old);
	}

	const InventoryLocation &getFormspecLocation()
	{
		return m_current_inventory_location;
	}

	void setFormspecPrepend(const std::string &formspecPrepend)
	{
		m_formspec_prepend = formspecPrepend;
	}

	// form_src is deleted by this GUIFormSpecMenu
	void setFormSource(IFormSource *form_src)
	{
		delete m_form_src;
		m_form_src = form_src;
	}

	// text_dst is deleted by this GUIFormSpecMenu
	void setTextDest(TextDest *text_dst)
	{
		delete m_text_dst;
		m_text_dst = text_dst;
	}

	void allowClose(bool value)
	{
		m_allowclose = value;
	}

	void lockSize(bool lock,v2u32 basescreensize=v2u32(0,0))
	{
		m_lock = lock;
		m_lockscreensize = basescreensize;
	}

	void removeChildren();
	void setInitialFocus();

	void setFocus(const std::string &elementname)
	{
		m_focused_element = elementname;
	}

	Client *getClient() const
	{
		return m_client;
	}

	const GUIInventoryList::ItemSpec *getSelectedItem() const
	{
		return m_selected_item;
	}

	const u16 getSelectedAmount() const
	{
		return m_selected_amount;
	}

	bool doTooltipAppendItemname() const
	{
		return m_tooltip_append_itemname;
	}

	void addHoveredItemTooltip(const std::string &name)
	{
		m_hovered_item_tooltips.emplace_back(name);
	}

	/*
		Remove and re-add (or reposition) stuff
	*/
	void regenerateGui(v2u32 screensize);

	GUIInventoryList::ItemSpec getItemAtPos(v2s32 p) const;
	void drawSelectedItem();
	void drawMenu();
	void updateSelectedItem();
	ItemStack verifySelectedItem();

	void acceptInput(FormspecQuitMode quitmode);
	bool preprocessEvent(const SEvent& event);
	bool OnEvent(const SEvent& event);
	bool doPause;
	bool pausesGame() { return doPause; }

	GUITable* getTable(const std::string &tablename);
	std::vector<std::string>* getDropDownValues(const std::string &name);

#ifdef __ANDROID__
	bool getAndroidUIInput();
#endif

protected:
	v2s32 getBasePos() const
	{
			return padding + offset + AbsoluteRect.UpperLeftCorner;
	}
	std::wstring getLabelByID(s32 id);
	std::string getNameByID(s32 id);
	const FieldSpec *getSpecByID(s32 id);
	v2s32 getElementBasePos(const std::vector<std::string> *v_pos);
	v2s32 getRealCoordinateBasePos(const std::vector<std::string> &v_pos);
	v2s32 getRealCoordinateGeometry(const std::vector<std::string> &v_geom);

	std::unordered_map<std::string, std::vector<StyleSpec>> theme_by_type;
	std::unordered_map<std::string, std::vector<StyleSpec>> theme_by_name;
	std::unordered_set<std::string> property_warned;

	StyleSpec getDefaultStyleForElement(const std::string &type,
			const std::string &name="", const std::string &parent_type="");
	std::array<StyleSpec, StyleSpec::NUM_STATES> getStyleForElement(const std::string &type,
			const std::string &name="", const std::string &parent_type="");

	v2s32 padding;
	v2f32 spacing;
	v2s32 imgsize;
	v2s32 offset;
	v2f32 pos_offset;
	std::stack<v2f32> container_stack;

	InventoryManager *m_invmgr;
	ISimpleTextureSource *m_tsrc;
	Client *m_client;

	std::string m_formspec_string;
	std::string m_formspec_prepend;
	InventoryLocation m_current_inventory_location;

	// Default true because we can't control regeneration on resizing, but
	// we can control cases when the formspec is shown intentionally.
	bool m_is_form_regenerated = true;

	std::vector<GUIInventoryList *> m_inventorylists;
	std::vector<ListRingSpec> m_inventory_rings;
	std::vector<gui::IGUIElement *> m_backgrounds;
	std::unordered_map<std::string, bool> field_close_on_enter;
	std::unordered_map<std::string, bool> m_dropdown_index_event;
	std::vector<FieldSpec> m_fields;
	std::vector<std::pair<FieldSpec, GUITable *>> m_tables;
	std::vector<std::pair<FieldSpec, gui::IGUICheckBox *>> m_checkboxes;
	std::map<std::string, TooltipSpec> m_tooltips;
	std::vector<std::pair<gui::IGUIElement *, TooltipSpec>> m_tooltip_rects;
	std::vector<std::pair<FieldSpec, GUIScrollBar *>> m_scrollbars;
	std::vector<std::pair<FieldSpec, std::vector<std::string>>> m_dropdowns;
	std::vector<gui::IGUIElement *> m_clickthrough_elements;
	std::vector<std::pair<std::string, GUIScrollContainer *>> m_scroll_containers;

	GUIInventoryList::ItemSpec *m_selected_item = nullptr;
	u16 m_selected_amount = 0;
	bool m_selected_dragging = false;
	ItemStack m_selected_swap;

	gui::IGUIStaticText *m_tooltip_element = nullptr;

	u64 m_tooltip_show_delay;
	bool m_tooltip_append_itemname;
	u64 m_hovered_time = 0;
	s32 m_old_tooltip_id = -1;

	bool m_auto_place = false;

	bool m_allowclose = true;
	bool m_lock = false;
	v2u32 m_lockscreensize;

	bool m_bgnonfullscreen;
	bool m_bgfullscreen;
	video::SColor m_bgcolor;
	video::SColor m_fullscreen_bgcolor;
	video::SColor m_default_tooltip_bgcolor;
	video::SColor m_default_tooltip_color;

private:
	IFormSource        *m_form_src;
	TextDest           *m_text_dst;
	std::string         m_last_formname;
	u16                 m_formspec_version = 1;
	std::string         m_focused_element = "";
	JoystickController *m_joystick;
	bool m_show_debug = false;

	typedef struct {
		bool explicit_size;
		bool real_coordinates;
		u8 simple_field_count;
		v2f invsize;
		v2s32 size;
		v2f32 offset;
		v2f32 anchor;
		core::rect<s32> rect;
		v2s32 basepos;
		v2u32 screensize;
		GUITable::TableOptions table_options;
		GUITable::TableColumns table_columns;
		gui::IGUIElement *current_parent = nullptr;

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
	} parserData;

	typedef struct {
		bool key_up;
		bool key_down;
		bool key_enter;
		bool key_escape;
	} fs_key_pendig;

	fs_key_pendig current_keys_pending;
	std::string current_field_enter_pending = "";
	std::vector<std::string> m_hovered_item_tooltips;

	void parseElement(parserData* data, const std::string &element);

	void parseSize(parserData* data, const std::string &element);
	void parseContainer(parserData* data, const std::string &element);
	void parseContainerEnd(parserData* data);
	void parseScrollContainer(parserData *data, const std::string &element);
	void parseScrollContainerEnd(parserData *data);
	void parseList(parserData* data, const std::string &element);
	void parseListRing(parserData* data, const std::string &element);
	void parseCheckbox(parserData* data, const std::string &element);
	void parseImage(parserData* data, const std::string &element);
	void parseAnimatedImage(parserData *data, const std::string &element);
	void parseItemImage(parserData* data, const std::string &element);
	void parseButton(parserData* data, const std::string &element,
			const std::string &typ);
	void parseBackground(parserData* data, const std::string &element);
	void parseTableOptions(parserData* data, const std::string &element);
	void parseTableColumns(parserData* data, const std::string &element);
	void parseTable(parserData* data, const std::string &element);
	void parseTextList(parserData* data, const std::string &element);
	void parseDropDown(parserData* data, const std::string &element);
	void parseFieldCloseOnEnter(parserData *data, const std::string &element);
	void parsePwdField(parserData* data, const std::string &element);
	void parseField(parserData* data, const std::string &element, const std::string &type);
	void createTextField(parserData *data, FieldSpec &spec,
		core::rect<s32> &rect, bool is_multiline);
	void parseSimpleField(parserData* data,std::vector<std::string> &parts);
	void parseTextArea(parserData* data,std::vector<std::string>& parts,
			const std::string &type);
	void parseHyperText(parserData *data, const std::string &element);
	void parseLabel(parserData* data, const std::string &element);
	void parseVertLabel(parserData* data, const std::string &element);
	void parseImageButton(parserData* data, const std::string &element,
			const std::string &type);
	void parseItemImageButton(parserData* data, const std::string &element);
	void parseTabHeader(parserData* data, const std::string &element);
	void parseBox(parserData* data, const std::string &element);
	void parseBackgroundColor(parserData* data, const std::string &element);
	void parseListColors(parserData* data, const std::string &element);
	void parseTooltip(parserData* data, const std::string &element);
	bool parseVersionDirect(const std::string &data);
	bool parseSizeDirect(parserData* data, const std::string &element);
	void parseScrollBar(parserData* data, const std::string &element);
	void parseScrollBarOptions(parserData *data, const std::string &element);
	bool parsePositionDirect(parserData *data, const std::string &element);
	void parsePosition(parserData *data, const std::string &element);
	bool parseAnchorDirect(parserData *data, const std::string &element);
	void parseAnchor(parserData *data, const std::string &element);
	bool parseStyle(parserData *data, const std::string &element, bool style_type);
	void parseSetFocus(const std::string &element);

	void tryClose();

	void showTooltip(const std::wstring &text, const irr::video::SColor &color,
		const irr::video::SColor &bgcolor);

	/**
	 * In formspec version < 2 the elements were not ordered properly. Some element
	 * types were drawn before others.
	 * This function sorts the elements in the old order for backwards compatibility.
	 */
	void legacySortElements(core::list<IGUIElement *>::Iterator from);

	int m_btn_height;
	gui::IGUIFont *m_font = nullptr;
};

class FormspecFormSource: public IFormSource
{
public:
	FormspecFormSource(const std::string &formspec):
		m_formspec(formspec)
	{
	}

	~FormspecFormSource() = default;

	void setForm(const std::string &formspec)
	{
		m_formspec = formspec;
	}

	const std::string &getForm() const
	{
		return m_formspec;
	}

	std::string m_formspec;
};
