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


#include "guiFormSpecMenu.h"
#include "constants.h"
#include "gamedef.h"
#include "keycode.h"
#include "strfnd.h"
#include <IGUICheckBox.h>
#include <IGUIEditBox.h>
#include <IGUIButton.h>
#include <IGUIStaticText.h>
#include <IGUIFont.h>
#include "log.h"
#include "tile.h" // ITextureSource
#include "util/string.h"
#include "util/numeric.h"

#include "gettext.h"

void drawItemStack(video::IVideoDriver *driver,
		gui::IGUIFont *font,
		const ItemStack &item,
		const core::rect<s32> &rect,
		const core::rect<s32> *clip,
		IGameDef *gamedef)
{
	if(item.empty())
		return;
	
	const ItemDefinition &def = item.getDefinition(gamedef->idef());
	video::ITexture *texture = def.inventory_texture;

	// Draw the inventory texture
	if(texture != NULL)
	{
		const video::SColor color(255,255,255,255);
		const video::SColor colors[] = {color,color,color,color};
		driver->draw2DImage(texture, rect,
			core::rect<s32>(core::position2d<s32>(0,0),
			core::dimension2di(texture->getOriginalSize())),
			clip, colors, true);
	}

	if(def.type == ITEM_TOOL && item.wear != 0)
	{
		// Draw a progressbar
		float barheight = rect.getHeight()/16;
		float barpad_x = rect.getWidth()/16;
		float barpad_y = rect.getHeight()/16;
		core::rect<s32> progressrect(
			rect.UpperLeftCorner.X + barpad_x,
			rect.LowerRightCorner.Y - barpad_y - barheight,
			rect.LowerRightCorner.X - barpad_x,
			rect.LowerRightCorner.Y - barpad_y);

		// Shrink progressrect by amount of tool damage
		float wear = item.wear / 65535.0;
		int progressmid =
			wear * progressrect.UpperLeftCorner.X +
			(1-wear) * progressrect.LowerRightCorner.X;

		// Compute progressbar color
		//   wear = 0.0: green
		//   wear = 0.5: yellow
		//   wear = 1.0: red
		video::SColor color(255,255,255,255);
		int wear_i = MYMIN(floor(wear * 600), 511);
		wear_i = MYMIN(wear_i + 10, 511);
		if(wear_i <= 255)
			color.set(255, wear_i, 255, 0);
		else
			color.set(255, 255, 511-wear_i, 0);

		core::rect<s32> progressrect2 = progressrect;
		progressrect2.LowerRightCorner.X = progressmid;
		driver->draw2DRectangle(color, progressrect2, clip);

		color = video::SColor(255,0,0,0);
		progressrect2 = progressrect;
		progressrect2.UpperLeftCorner.X = progressmid;
		driver->draw2DRectangle(color, progressrect2, clip);
	}

	if(font != NULL && item.count >= 2)
	{
		// Get the item count as a string
		std::string text = itos(item.count);
		v2u32 dim = font->getDimension(narrow_to_wide(text).c_str());
		v2s32 sdim(dim.X,dim.Y);

		core::rect<s32> rect2(
			/*rect.UpperLeftCorner,
			core::dimension2d<u32>(rect.getWidth(), 15)*/
			rect.LowerRightCorner - sdim,
			sdim
		);

		video::SColor bgcolor(128,0,0,0);
		driver->draw2DRectangle(bgcolor, rect2, clip);

		video::SColor color(255,255,255,255);
		font->draw(text.c_str(), rect2, color, false, false, clip);
	}
}

/*
	GUIFormSpecMenu
*/

GUIFormSpecMenu::GUIFormSpecMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr,
		InventoryManager *invmgr,
		IGameDef *gamedef
):
	GUIModalMenu(env, parent, id, menumgr),
	m_invmgr(invmgr),
	m_gamedef(gamedef),
	m_form_src(NULL),
	m_text_dst(NULL),
	m_selected_item(NULL),
	m_selected_amount(0),
	m_selected_dragging(false),
	m_tooltip_element(NULL)
{
}

GUIFormSpecMenu::~GUIFormSpecMenu()
{
	removeChildren();

	delete m_selected_item;
	delete m_form_src;
	delete m_text_dst;
}

void GUIFormSpecMenu::removeChildren()
{
	const core::list<gui::IGUIElement*> &children = getChildren();
	core::list<gui::IGUIElement*> children_copy;
	for(core::list<gui::IGUIElement*>::ConstIterator
			i = children.begin(); i != children.end(); i++)
	{
		children_copy.push_back(*i);
	}
	for(core::list<gui::IGUIElement*>::Iterator
			i = children_copy.begin();
			i != children_copy.end(); i++)
	{
		(*i)->remove();
	}
	/*{
		gui::IGUIElement *e = getElementFromId(256);
		if(e != NULL)
			e->remove();
	}*/
	if(m_tooltip_element)
	{
		m_tooltip_element->remove();
		m_tooltip_element = NULL;
	}
}

void GUIFormSpecMenu::regenerateGui(v2u32 screensize)
{
	// Remove children
	removeChildren();
	
	v2s32 size(100,100);
	s32 helptext_h = 15;
	core::rect<s32> rect;

	// Base position of contents of form
	v2s32 basepos = getBasePos();
	// State of basepos, 0 = not set, 1= set by formspec, 2 = set by size[] element
	// Used to adjust form size automatically if needed
	// A proceed button is added if there is no size[] element
	int bp_set = 0;
	
	/* Convert m_init_draw_spec to m_inventorylists */
	
	m_inventorylists.clear();
	m_images.clear();
	m_fields.clear();

	Strfnd f(m_formspec_string);
	while(f.atend() == false)
	{
		std::string type = trim(f.next("["));
		if(type == "invsize" || type == "size")
		{
			v2f invsize;
			invsize.X = stof(f.next(","));
			if(type == "size")
			{
				invsize.Y = stof(f.next("]"));
			}
			else{
				invsize.Y = stof(f.next(";"));
				f.next("]");
			}
			infostream<<"Form size ("<<invsize.X<<","<<invsize.Y<<")"<<std::endl;

			padding = v2s32(screensize.Y/40, screensize.Y/40);
			spacing = v2s32(screensize.Y/12, screensize.Y/13);
			imgsize = v2s32(screensize.Y/15, screensize.Y/15);
			size = v2s32(
				padding.X*2+spacing.X*(invsize.X-1.0)+imgsize.X,
				padding.Y*2+spacing.Y*(invsize.Y-1.0)+imgsize.Y + (helptext_h-5)
			);
			rect = core::rect<s32>(
					screensize.X/2 - size.X/2,
					screensize.Y/2 - size.Y/2,
					screensize.X/2 + size.X/2,
					screensize.Y/2 + size.Y/2
			);
			DesiredRect = rect;
			recalculateAbsolutePosition(false);
			basepos = getBasePos();
			bp_set = 2;
		}
		else if(type == "list")
		{
			std::string name = f.next(";");
			InventoryLocation loc;
			if(name == "context" || name == "current_name")
				loc = m_current_inventory_location;
			else
				loc.deSerialize(name);
			std::string listname = f.next(";");
			v2s32 pos = basepos;
			pos.X += stof(f.next(",")) * (float)spacing.X;
			pos.Y += stof(f.next(";")) * (float)spacing.Y;
			v2s32 geom;
			geom.X = stoi(f.next(","));
			geom.Y = stoi(f.next(";"));
			infostream<<"list inv="<<name<<", listname="<<listname
					<<", pos=("<<pos.X<<","<<pos.Y<<")"
					<<", geom=("<<geom.X<<","<<geom.Y<<")"
					<<std::endl;
			std::string start_i_s = f.next("]");
			s32 start_i = 0;
			if(start_i_s != "")
				start_i = stoi(start_i_s);
			if(bp_set != 2)
				errorstream<<"WARNING: invalid use of list without a size[] element"<<std::endl;
			m_inventorylists.push_back(ListDrawSpec(loc, listname, pos, geom, start_i));
		}
		else if(type == "image")
		{
			v2s32 pos = basepos;
			pos.X += stof(f.next(",")) * (float)spacing.X;
			pos.Y += stof(f.next(";")) * (float)spacing.Y;
			v2s32 geom;
			geom.X = stof(f.next(",")) * (float)imgsize.X;
			geom.Y = stof(f.next(";")) * (float)imgsize.Y;
			std::string name = f.next("]");
			infostream<<"image name="<<name
					<<", pos=("<<pos.X<<","<<pos.Y<<")"
					<<", geom=("<<geom.X<<","<<geom.Y<<")"
					<<std::endl;
			if(bp_set != 2)
				errorstream<<"WARNING: invalid use of button without a size[] element"<<std::endl;
			m_images.push_back(ImageDrawSpec(name, pos, geom));
		}
		else if(type == "field")
		{
			std::string fname = f.next(";");
			std::string flabel = f.next(";");
			
			if(fname.find(",") == std::string::npos && flabel.find(",") == std::string::npos)
			{
				if(!bp_set)
				{
					rect = core::rect<s32>(
						screensize.X/2 - 580/2,
						screensize.Y/2 - 300/2,
						screensize.X/2 + 580/2,
						screensize.Y/2 + 300/2
					);
					DesiredRect = rect;
					recalculateAbsolutePosition(false);
					basepos = getBasePos();
					bp_set = 1;
				}
				else if(bp_set == 2)
					errorstream<<"WARNING: invalid use of unpositioned field in inventory"<<std::endl;

				v2s32 pos = basepos;
				pos.Y = ((m_fields.size()+2)*60);
				v2s32 size = DesiredRect.getSize();
				rect = core::rect<s32>(size.X/2-150, pos.Y, (size.X/2-150)+300, pos.Y+30);
			}
			else
			{
				v2s32 pos;
				pos.X = stof(fname.substr(0,fname.find(","))) * (float)spacing.X;
				pos.Y = stof(fname.substr(fname.find(",")+1)) * (float)spacing.Y;
				v2s32 geom;
				geom.X = (stof(flabel.substr(0,flabel.find(","))) * (float)spacing.X)-(spacing.X-imgsize.X);
				pos.Y += (stof(flabel.substr(flabel.find(",")+1)) * (float)imgsize.Y)/2;

				rect = core::rect<s32>(pos.X, pos.Y-15, pos.X+geom.X, pos.Y+15);
				
				fname = f.next(";");
				flabel = f.next(";");
				if(bp_set != 2)
					errorstream<<"WARNING: invalid use of positioned field without a size[] element"<<std::endl;
				
			}

			std::string odefault = f.next("]");
			std::string fdefault;
			
			// fdefault may contain a variable reference, which
			// needs to be resolved from the node metadata
			if(m_form_src)
				fdefault = m_form_src->resolveText(odefault);
			else
				fdefault = odefault;

			FieldSpec spec = FieldSpec(
				narrow_to_wide(fname.c_str()),
				narrow_to_wide(flabel.c_str()),
				narrow_to_wide(fdefault.c_str()),
				258+m_fields.size()
			);
			
			// three cases: field and no label, label and no field, label and field
			if (flabel == "") 
			{
				spec.send = true;
				gui::IGUIElement *e = Environment->addEditBox(spec.fdefault.c_str(), rect, true, this, spec.fid);
				Environment->setFocus(e);

				irr::SEvent evt;
				evt.EventType = EET_KEY_INPUT_EVENT;
				evt.KeyInput.Key = KEY_END;
				evt.KeyInput.PressedDown = true;
				e->OnEvent(evt);
			}
			else if (fname == "")
			{
				// set spec field id to 0, this stops submit searching for a value that isn't there
				Environment->addStaticText(spec.flabel.c_str(), rect, false, true, this, spec.fid);
			}
			else
			{
				spec.send = true;
				gui::IGUIElement *e = Environment->addEditBox(spec.fdefault.c_str(), rect, true, this, spec.fid);
				Environment->setFocus(e);
				rect.UpperLeftCorner.Y -= 15;
				rect.LowerRightCorner.Y -= 15;
				Environment->addStaticText(spec.flabel.c_str(), rect, false, true, this, 0);

				irr::SEvent evt;
				evt.EventType = EET_KEY_INPUT_EVENT;
				evt.KeyInput.Key = KEY_END;
				evt.KeyInput.PressedDown = true;
				e->OnEvent(evt);
			}
			
			m_fields.push_back(spec);
		}
		else if(type == "label")
		{
			v2s32 pos = padding;
			pos.X += stof(f.next(",")) * (float)spacing.X;
			pos.Y += stof(f.next(";")) * (float)spacing.Y;

			rect = core::rect<s32>(pos.X, pos.Y+((imgsize.Y/2)-15), pos.X+300, pos.Y+((imgsize.Y/2)+15));
			
			std::string flabel = f.next("]");
			if(bp_set != 2)
				errorstream<<"WARNING: invalid use of label without a size[] element"<<std::endl;

			FieldSpec spec = FieldSpec(
				narrow_to_wide(""),
				narrow_to_wide(flabel.c_str()),
				narrow_to_wide(""),
				258+m_fields.size()
			);
			Environment->addStaticText(spec.flabel.c_str(), rect, false, true, this, spec.fid);
			m_fields.push_back(spec);
		}
		else if(type == "button" || type == "button_exit")
		{
			v2s32 pos = padding;
			pos.X += stof(f.next(",")) * (float)spacing.X;
			pos.Y += stof(f.next(";")) * (float)spacing.Y;
			v2s32 geom;
			geom.X = (stof(f.next(",")) * (float)spacing.X)-(spacing.X-imgsize.X);
			pos.Y += (stof(f.next(";")) * (float)imgsize.Y)/2;

			rect = core::rect<s32>(pos.X, pos.Y-15, pos.X+geom.X, pos.Y+15);
			
			std::string fname = f.next(";");
			std::string flabel = f.next("]");
			if(bp_set != 2)
				errorstream<<"WARNING: invalid use of button without a size[] element"<<std::endl;

			FieldSpec spec = FieldSpec(
				narrow_to_wide(fname.c_str()),
				narrow_to_wide(flabel.c_str()),
				narrow_to_wide(""),
				258+m_fields.size()
			);
			spec.is_button = true;
			if(type == "button_exit")
				spec.is_exit = true;
			Environment->addButton(rect, this, spec.fid, spec.flabel.c_str());
			m_fields.push_back(spec);
		}
		else if(type == "image_button" || type == "image_button_exit")
		{
			v2s32 pos = padding;
			pos.X += stof(f.next(",")) * (float)spacing.X;
			pos.Y += stof(f.next(";")) * (float)spacing.Y;
			v2s32 geom;
			geom.X = (stof(f.next(",")) * (float)spacing.X)-(spacing.X-imgsize.X);
			geom.Y = (stof(f.next(";")) * (float)spacing.Y)-(spacing.Y-imgsize.Y);

			rect = core::rect<s32>(pos.X, pos.Y, pos.X+geom.X, pos.Y+geom.Y);
			
			std::string fimage = f.next(";");
			std::string fname = f.next(";");
			std::string flabel = f.next("]");
			if(bp_set != 2)
				errorstream<<"WARNING: invalid use of image_button without a size[] element"<<std::endl;

			FieldSpec spec = FieldSpec(
				narrow_to_wide(fname.c_str()),
				narrow_to_wide(flabel.c_str()),
				narrow_to_wide(fimage.c_str()),
				258+m_fields.size()
			);
			spec.is_button = true;
			if(type == "image_button_exit")
				spec.is_exit = true;
			
			video::ITexture *texture = m_gamedef->tsrc()->getTextureRaw(fimage);
			gui::IGUIButton *e = Environment->addButton(rect, this, spec.fid, spec.flabel.c_str());
			e->setImage(texture);
			e->setPressedImage(texture);
			e->setScaleImage(true);
			
			m_fields.push_back(spec);
		}
		else
		{
			// Ignore others
			std::string ts = f.next("]");
			infostream<<"Unknown DrawSpec: type="<<type<<", data=\""<<ts<<"\""
					<<std::endl;
		}
	}

	// If there's inventory, put the usage string at the bottom
	if (m_inventorylists.size())
	{
		changeCtype("");
		core::rect<s32> rect(0, 0, size.X-padding.X*2, helptext_h);
		rect = rect + v2s32(size.X/2 - rect.getWidth()/2,
				size.Y-rect.getHeight()-5);
		const wchar_t *text = wgettext("Left click: Move all items, Right click: Move single item");
		Environment->addStaticText(text, rect, false, true, this, 256);
		changeCtype("C");
	}
	// If there's fields, add a Proceed button
	if (m_fields.size() && bp_set != 2) 
	{
		// if the size wasn't set by an invsize[] or size[] adjust it now to fit all the fields
		rect = core::rect<s32>(
			screensize.X/2 - 580/2,
			screensize.Y/2 - 300/2,
			screensize.X/2 + 580/2,
			screensize.Y/2 + 240/2+(m_fields.size()*60)
		);
		DesiredRect = rect;
		recalculateAbsolutePosition(false);
		basepos = getBasePos();

		changeCtype("");
		{
			v2s32 pos = basepos;
			pos.Y = ((m_fields.size()+2)*60);

			v2s32 size = DesiredRect.getSize();
			rect = core::rect<s32>(size.X/2-70, pos.Y, (size.X/2-70)+140, pos.Y+30);
			Environment->addButton(rect, this, 257, wgettext("Proceed"));
		}
		changeCtype("C");
	}
	// Add tooltip
	{
		// Note: parent != this so that the tooltip isn't clipped by the menu rectangle
		m_tooltip_element = Environment->addStaticText(L"",core::rect<s32>(0,0,110,18));
		m_tooltip_element->enableOverrideColor(true);
		m_tooltip_element->setBackgroundColor(video::SColor(255,110,130,60));
		m_tooltip_element->setDrawBackground(true);
		m_tooltip_element->setDrawBorder(true);
		m_tooltip_element->setOverrideColor(video::SColor(255,255,255,255));
		m_tooltip_element->setTextAlignment(gui::EGUIA_CENTER, gui::EGUIA_CENTER);
		m_tooltip_element->setWordWrap(false);
	}
}

GUIFormSpecMenu::ItemSpec GUIFormSpecMenu::getItemAtPos(v2s32 p) const
{
	core::rect<s32> imgrect(0,0,imgsize.X,imgsize.Y);
	
	for(u32 i=0; i<m_inventorylists.size(); i++)
	{
		const ListDrawSpec &s = m_inventorylists[i];

		for(s32 i=0; i<s.geom.X*s.geom.Y; i++)
		{
			s32 item_i = i + s.start_item_i;
			s32 x = (i%s.geom.X) * spacing.X;
			s32 y = (i/s.geom.X) * spacing.Y;
			v2s32 p0(x,y);
			core::rect<s32> rect = imgrect + s.pos + p0;
			if(rect.isPointInside(p))
			{
				return ItemSpec(s.inventoryloc, s.listname, item_i);
			}
		}
	}

	return ItemSpec(InventoryLocation(), "", -1);
}

void GUIFormSpecMenu::drawList(const ListDrawSpec &s, int phase)
{
	video::IVideoDriver* driver = Environment->getVideoDriver();

	// Get font
	gui::IGUIFont *font = NULL;
	gui::IGUISkin* skin = Environment->getSkin();
	if (skin)
		font = skin->getFont();
	
	Inventory *inv = m_invmgr->getInventory(s.inventoryloc);
	if(!inv){
		infostream<<"GUIFormSpecMenu::drawList(): WARNING: "
				<<"The inventory location "
				<<"\""<<s.inventoryloc.dump()<<"\" doesn't exist"
				<<std::endl;
		return;
	}
	InventoryList *ilist = inv->getList(s.listname);
	if(!ilist){
		infostream<<"GUIFormSpecMenu::drawList(): WARNING: "
				<<"The inventory list \""<<s.listname<<"\" @ \""
				<<s.inventoryloc.dump()<<"\" doesn't exist"
				<<std::endl;
		return;
	}
	
	core::rect<s32> imgrect(0,0,imgsize.X,imgsize.Y);
	
	for(s32 i=0; i<s.geom.X*s.geom.Y; i++)
	{
		u32 item_i = i + s.start_item_i;
		if(item_i >= ilist->getSize())
			break;
		s32 x = (i%s.geom.X) * spacing.X;
		s32 y = (i/s.geom.X) * spacing.Y;
		v2s32 p(x,y);
		core::rect<s32> rect = imgrect + s.pos + p;
		ItemStack item;
		if(ilist)
			item = ilist->getItem(item_i);

		bool selected = m_selected_item
			&& m_invmgr->getInventory(m_selected_item->inventoryloc) == inv
			&& m_selected_item->listname == s.listname
			&& m_selected_item->i == i;
		bool hovering = rect.isPointInside(m_pointer);

		if(phase == 0)
		{
			if(hovering && m_selected_item)
			{
				video::SColor bgcolor(255,192,192,192);
				driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
			}
			else
			{
				video::SColor bgcolor(255,128,128,128);
				driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
			}
		}

		if(phase == 1)
		{
			// Draw item stack
			if(selected)
			{
				item.takeItem(m_selected_amount);
			}
			if(!item.empty())
			{
				drawItemStack(driver, font, item,
						rect, &AbsoluteClippingRect, m_gamedef);
			}

			// Draw tooltip
			std::string tooltip_text = "";
			if(hovering && !m_selected_item)
				tooltip_text = item.getDefinition(m_gamedef->idef()).description;
			if(tooltip_text != "")
			{
				m_tooltip_element->setVisible(true);
				this->bringToFront(m_tooltip_element);
				m_tooltip_element->setText(narrow_to_wide(tooltip_text).c_str());
				s32 tooltip_x = m_pointer.X + 15;
				s32 tooltip_y = m_pointer.Y + 15;
				s32 tooltip_width = m_tooltip_element->getTextWidth() + 15;
				s32 tooltip_height = m_tooltip_element->getTextHeight() + 5;
				m_tooltip_element->setRelativePosition(core::rect<s32>(
						core::position2d<s32>(tooltip_x, tooltip_y),
						core::dimension2d<s32>(tooltip_width, tooltip_height)));
			}
		}
	}
}

void GUIFormSpecMenu::drawSelectedItem()
{
	if(!m_selected_item)
		return;

	video::IVideoDriver* driver = Environment->getVideoDriver();

	// Get font
	gui::IGUIFont *font = NULL;
	gui::IGUISkin* skin = Environment->getSkin();
	if (skin)
		font = skin->getFont();
	
	Inventory *inv = m_invmgr->getInventory(m_selected_item->inventoryloc);
	assert(inv);
	InventoryList *list = inv->getList(m_selected_item->listname);
	assert(list);
	ItemStack stack = list->getItem(m_selected_item->i);
	stack.count = m_selected_amount;

	core::rect<s32> imgrect(0,0,imgsize.X,imgsize.Y);
	core::rect<s32> rect = imgrect + (m_pointer - imgrect.getCenter());
	drawItemStack(driver, font, stack, rect, NULL, m_gamedef);
}

void GUIFormSpecMenu::drawMenu()
{
	if(m_form_src){
		std::string newform = m_form_src->getForm();
		if(newform != m_formspec_string){
			m_formspec_string = newform;
			regenerateGui(m_screensize_old);
		}
	}

	updateSelectedItem();

	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	
	video::SColor bgcolor(140,0,0,0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);

	m_tooltip_element->setVisible(false);

	/*
		Draw items
		Phase 0: Item slot rectangles
		Phase 1: Item images; prepare tooltip
	*/
	
	for(int phase=0; phase<=1; phase++)
	for(u32 i=0; i<m_inventorylists.size(); i++)
	{
		drawList(m_inventorylists[i], phase);
	}

	for(u32 i=0; i<m_images.size(); i++)
	{
		const ImageDrawSpec &spec = m_images[i];
		video::ITexture *texture =
				m_gamedef->tsrc()->getTextureRaw(spec.name);
		// Image size on screen
		core::rect<s32> imgrect(0, 0, spec.geom.X, spec.geom.Y);
		// Image rectangle on screen
		core::rect<s32> rect = imgrect + spec.pos;
		const video::SColor color(255,255,255,255);
		const video::SColor colors[] = {color,color,color,color};
		driver->draw2DImage(texture, rect,
			core::rect<s32>(core::position2d<s32>(0,0),
					core::dimension2di(texture->getOriginalSize())),
			NULL/*&AbsoluteClippingRect*/, colors, true);
	}

	/*
		Draw dragged item stack
	*/
	drawSelectedItem();

	/*
		Call base class
	*/
	gui::IGUIElement::draw();
}

void GUIFormSpecMenu::updateSelectedItem()
{
	// WARNING: BLACK MAGIC
	// See if there is a stack suited for our current guess.
	// If such stack does not exist, clear the guess.
	if(m_selected_content_guess.name != "")
	{
		bool found = false;
		for(u32 i=0; i<m_inventorylists.size() && !found; i++){
			const ListDrawSpec &s = m_inventorylists[i];
			Inventory *inv = m_invmgr->getInventory(s.inventoryloc);
			if(!inv)
				continue;
			InventoryList *list = inv->getList(s.listname);
			if(!list)
				continue;
			for(s32 i=0; i<s.geom.X*s.geom.Y && !found; i++){
				u32 item_i = i + s.start_item_i;
				if(item_i >= list->getSize())
					continue;
				ItemStack stack = list->getItem(item_i);
				if(stack.name == m_selected_content_guess.name &&
						stack.count == m_selected_content_guess.count){
					found = true;
					if(m_selected_item){
						// If guessed stack is already selected, all is fine
						if(m_selected_item->inventoryloc == s.inventoryloc &&
								m_selected_item->listname == s.listname &&
								m_selected_item->i == (s32)item_i &&
								m_selected_amount == stack.count){
							break;
						}
						delete m_selected_item;
						m_selected_item = NULL;
					}
					infostream<<"Client: Changing selected content guess to "
							<<s.inventoryloc.dump()<<" "<<s.listname
							<<" "<<item_i<<std::endl;
					m_selected_item = new ItemSpec(s.inventoryloc, s.listname, item_i);
					m_selected_amount = stack.count;
					break;
				}
			}
		}
		if(!found){
			infostream<<"Client: Discarding selected content guess: "
					<<m_selected_content_guess.getItemString()<<std::endl;
			m_selected_content_guess.name = "";
		}
	}
	// If the selected stack has become empty for some reason, deselect it.
	// If the selected stack has become smaller, adjust m_selected_amount.
	if(m_selected_item)
	{
		bool selection_valid = false;
		if(m_selected_item->isValid())
		{
			Inventory *inv = m_invmgr->getInventory(m_selected_item->inventoryloc);
			if(inv)
			{
				InventoryList *list = inv->getList(m_selected_item->listname);
				if(list && (u32) m_selected_item->i < list->getSize())
				{
					ItemStack stack = list->getItem(m_selected_item->i);
					if(m_selected_amount > stack.count)
						m_selected_amount = stack.count;
					if(!stack.empty())
						selection_valid = true;
				}
			}
		}
		if(!selection_valid)
		{
			delete m_selected_item;
			m_selected_item = NULL;
			m_selected_amount = 0;
			m_selected_dragging = false;
		}
	}

	// If craftresult is nonempty and nothing else is selected, select it now.
	if(!m_selected_item)
	{
		for(u32 i=0; i<m_inventorylists.size(); i++)
		{
			const ListDrawSpec &s = m_inventorylists[i];
			if(s.listname == "craftpreview")
			{
				Inventory *inv = m_invmgr->getInventory(s.inventoryloc);
				InventoryList *list = inv->getList("craftresult");
				if(list && list->getSize() >= 1 && !list->getItem(0).empty())
				{
					m_selected_item = new ItemSpec;
					m_selected_item->inventoryloc = s.inventoryloc;
					m_selected_item->listname = "craftresult";
					m_selected_item->i = 0;
					m_selected_amount = 0;
					m_selected_dragging = false;
					break;
				}
			}
		}
	}

	// If craftresult is selected, keep the whole stack selected
	if(m_selected_item && m_selected_item->listname == "craftresult")
	{
		Inventory *inv = m_invmgr->getInventory(m_selected_item->inventoryloc);
		assert(inv);
		InventoryList *list = inv->getList(m_selected_item->listname);
		assert(list);
		m_selected_amount = list->getItem(m_selected_item->i).count;
	}
}

void GUIFormSpecMenu::acceptInput()
{
	if(m_text_dst)
	{
		std::map<std::string, std::string> fields;
		gui::IGUIElement *e;
		for(u32 i=0; i<m_fields.size(); i++)
		{
			const FieldSpec &s = m_fields[i];
			if(s.send) 
			{
				if(s.is_button)
				{
					fields[wide_to_narrow(s.fname.c_str())] = wide_to_narrow(s.flabel.c_str());
				}
				else
				{
					e = getElementFromId(s.fid);
					if(e != NULL)
					{
						fields[wide_to_narrow(s.fname.c_str())] = wide_to_narrow(e->getText());
					}
				}
			}
		}
		m_text_dst->gotText(fields);
	}
}

bool GUIFormSpecMenu::OnEvent(const SEvent& event)
{
	if(event.EventType==EET_KEY_INPUT_EVENT)
	{
		KeyPress kp(event.KeyInput);
		if (event.KeyInput.PressedDown && (kp == EscapeKey ||
			kp == getKeySetting("keymap_inventory")))
		{
			quitMenu();
			return true;
		}
		if(event.KeyInput.Key==KEY_RETURN && event.KeyInput.PressedDown)
		{
			acceptInput();
			quitMenu();
			return true;
		}
	}
	if(event.EventType==EET_MOUSE_INPUT_EVENT
			&& event.MouseInput.Event == EMIE_MOUSE_MOVED)
	{
		// Mouse moved
		m_pointer = v2s32(event.MouseInput.X, event.MouseInput.Y);
	}
	if(event.EventType==EET_MOUSE_INPUT_EVENT
			&& event.MouseInput.Event != EMIE_MOUSE_MOVED)
	{
		// Mouse event other than movement

		v2s32 p(event.MouseInput.X, event.MouseInput.Y);
		m_pointer = p;

		// Get selected item and hovered/clicked item (s)

		updateSelectedItem();
		ItemSpec s = getItemAtPos(p);

		Inventory *inv_selected = NULL;
		Inventory *inv_s = NULL;

		if(m_selected_item)
		{
			inv_selected = m_invmgr->getInventory(m_selected_item->inventoryloc);
			assert(inv_selected);
			assert(inv_selected->getList(m_selected_item->listname) != NULL);
		}

		u32 s_count = 0;

		if(s.isValid())
		do{ // breakable
			inv_s = m_invmgr->getInventory(s.inventoryloc);

			if(!inv_s){
				errorstream<<"InventoryMenu: The selected inventory location "
						<<"\""<<s.inventoryloc.dump()<<"\" doesn't exist"
						<<std::endl;
				s.i = -1;  // make it invalid again
				break;
			}

			InventoryList *list = inv_s->getList(s.listname);
			if(list == NULL){
				verbosestream<<"InventoryMenu: The selected inventory list \""
						<<s.listname<<"\" does not exist"<<std::endl;
				s.i = -1;  // make it invalid again
				break;
			}

			if((u32)s.i >= list->getSize()){
				infostream<<"InventoryMenu: The selected inventory list \""
						<<s.listname<<"\" is too small (i="<<s.i<<", size="
						<<list->getSize()<<")"<<std::endl;
				s.i = -1;  // make it invalid again
				break;
			}

			s_count = list->getItem(s.i).count;
		}while(0);

		bool identical = (m_selected_item != NULL) && s.isValid() &&
			(inv_selected == inv_s) &&
			(m_selected_item->listname == s.listname) &&
			(m_selected_item->i == s.i);

		// buttons: 0 = left, 1 = right, 2 = middle
		// up/down: 0 = down (press), 1 = up (release), 2 = unknown event
		int button = 0;
		int updown = 2;
		if(event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN)
			{ button = 0; updown = 0; }
		else if(event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN)
			{ button = 1; updown = 0; }
		else if(event.MouseInput.Event == EMIE_MMOUSE_PRESSED_DOWN)
			{ button = 2; updown = 0; }
		else if(event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP)
			{ button = 0; updown = 1; }
		else if(event.MouseInput.Event == EMIE_RMOUSE_LEFT_UP)
			{ button = 1; updown = 1; }
		else if(event.MouseInput.Event == EMIE_MMOUSE_LEFT_UP)
			{ button = 2; updown = 1; }

		// Set this number to a positive value to generate a move action
		// from m_selected_item to s.
		u32 move_amount = 0;

		// Set this number to a positive value to generate a drop action
		// from m_selected_item.
		u32 drop_amount = 0;

		// Set this number to a positive value to generate a craft action at s.
		u32 craft_amount = 0;

		if(updown == 0)
		{
			// Some mouse button has been pressed

			//infostream<<"Mouse button "<<button<<" pressed at p=("
			//	<<p.X<<","<<p.Y<<")"<<std::endl;

			m_selected_dragging = false;

			if(s.isValid() && s.listname == "craftpreview")
			{
				// Craft preview has been clicked: craft
				craft_amount = (button == 2 ? 10 : 1);
			}
			else if(m_selected_item == NULL)
			{
				if(s_count != 0)
				{
					// Non-empty stack has been clicked: select it
					m_selected_item = new ItemSpec(s);

					if(button == 1)  // right
						m_selected_amount = (s_count + 1) / 2;
					else if(button == 2)  // middle
						m_selected_amount = MYMIN(s_count, 10);
					else  // left
						m_selected_amount = s_count;

					m_selected_dragging = true;
				}
			}
			else  // m_selected_item != NULL
			{
				assert(m_selected_amount >= 1);

				if(s.isValid())
				{
					// Clicked a slot: move
					if(button == 1)  // right
						move_amount = 1;
					else if(button == 2)  // middle
						move_amount = MYMIN(m_selected_amount, 10);
					else  // left
						move_amount = m_selected_amount;

					if(identical)
					{
						if(move_amount >= m_selected_amount)
							m_selected_amount = 0;
						else
							m_selected_amount -= move_amount;
						move_amount = 0;
					}
				}
				else if(getAbsoluteClippingRect().isPointInside(m_pointer))
				{
					// Clicked somewhere else: deselect
					m_selected_amount = 0;
				}
				else
				{
					// Clicked outside of the window: drop
					if(button == 1)  // right
						drop_amount = 1;
					else if(button == 2)  // middle
						drop_amount = MYMIN(m_selected_amount, 10);
					else  // left
						drop_amount = m_selected_amount;
				}
			}
		}
		else if(updown == 1)
		{
			// Some mouse button has been released

			//infostream<<"Mouse button "<<button<<" released at p=("
			//	<<p.X<<","<<p.Y<<")"<<std::endl;

			if(m_selected_item != NULL && m_selected_dragging && s.isValid())
			{
				if(!identical)
				{
					// Dragged to different slot: move all selected
					move_amount = m_selected_amount;
				}
			}
			else if(m_selected_item != NULL && m_selected_dragging &&
				!(getAbsoluteClippingRect().isPointInside(m_pointer)))
			{
				// Dragged outside of window: drop all selected
				drop_amount = m_selected_amount;
			}

			m_selected_dragging = false;
		}

		// Possibly send inventory action to server
		if(move_amount > 0)
		{
			// Send IACTION_MOVE

			assert(m_selected_item && m_selected_item->isValid());
			assert(s.isValid());

			assert(inv_selected && inv_s);
			InventoryList *list_from = inv_selected->getList(m_selected_item->listname);
			InventoryList *list_to = inv_s->getList(s.listname);
			assert(list_from && list_to);
			ItemStack stack_from = list_from->getItem(m_selected_item->i);
			ItemStack stack_to = list_to->getItem(s.i);

			// Check how many items can be moved
			move_amount = stack_from.count = MYMIN(move_amount, stack_from.count);
			ItemStack leftover = stack_to.addItem(stack_from, m_gamedef->idef());
			// If source stack cannot be added to destination stack at all,
			// they are swapped
			if(leftover.count == stack_from.count && leftover.name == stack_from.name)
			{
				m_selected_amount = stack_to.count;
				// In case the server doesn't directly swap them but instead
				// moves stack_to somewhere else, set this
				m_selected_content_guess = stack_to;
				m_selected_content_guess_inventory = s.inventoryloc;
			}
			// Source stack goes fully into destination stack
			else if(leftover.empty())
			{
				m_selected_amount -= move_amount;
				m_selected_content_guess = ItemStack(); // Clear
			}
			// Source stack goes partly into destination stack
			else
			{
				move_amount -= leftover.count;
				m_selected_amount -= move_amount;
				m_selected_content_guess = ItemStack(); // Clear
			}

			infostream<<"Handing IACTION_MOVE to manager"<<std::endl;
			IMoveAction *a = new IMoveAction();
			a->count = move_amount;
			a->from_inv = m_selected_item->inventoryloc;
			a->from_list = m_selected_item->listname;
			a->from_i = m_selected_item->i;
			a->to_inv = s.inventoryloc;
			a->to_list = s.listname;
			a->to_i = s.i;
			m_invmgr->inventoryAction(a);
		}
		else if(drop_amount > 0)
		{
			m_selected_content_guess = ItemStack(); // Clear

			// Send IACTION_DROP

			assert(m_selected_item && m_selected_item->isValid());
			assert(inv_selected);
			InventoryList *list_from = inv_selected->getList(m_selected_item->listname);
			assert(list_from);
			ItemStack stack_from = list_from->getItem(m_selected_item->i);

			// Check how many items can be dropped
			drop_amount = stack_from.count = MYMIN(drop_amount, stack_from.count);
			assert(drop_amount > 0 && drop_amount <= m_selected_amount);
			m_selected_amount -= drop_amount;

			infostream<<"Handing IACTION_DROP to manager"<<std::endl;
			IDropAction *a = new IDropAction();
			a->count = drop_amount;
			a->from_inv = m_selected_item->inventoryloc;
			a->from_list = m_selected_item->listname;
			a->from_i = m_selected_item->i;
			m_invmgr->inventoryAction(a);
		}
		else if(craft_amount > 0)
		{
			m_selected_content_guess = ItemStack(); // Clear

			// Send IACTION_CRAFT

			assert(s.isValid());
			assert(inv_s);

			infostream<<"Handing IACTION_CRAFT to manager"<<std::endl;
			ICraftAction *a = new ICraftAction();
			a->count = craft_amount;
			a->craft_inv = s.inventoryloc;
			m_invmgr->inventoryAction(a);
		}

		// If m_selected_amount has been decreased to zero, deselect
		if(m_selected_amount == 0)
		{
			delete m_selected_item;
			m_selected_item = NULL;
			m_selected_amount = 0;
			m_selected_dragging = false;
			m_selected_content_guess = ItemStack();
		}
	}
	if(event.EventType==EET_GUI_EVENT)
	{
		if(event.GUIEvent.EventType==gui::EGET_ELEMENT_FOCUS_LOST
				&& isVisible())
		{
			if(!canTakeFocus(event.GUIEvent.Element))
			{
				infostream<<"GUIFormSpecMenu: Not allowing focus change."
						<<std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if(event.GUIEvent.EventType==gui::EGET_BUTTON_CLICKED)
		{
			switch(event.GUIEvent.Caller->getID())
			{
			case 257:
				acceptInput();
				quitMenu();
				// quitMenu deallocates menu
				return true;
			}
			// find the element that was clicked
			for(u32 i=0; i<m_fields.size(); i++)
			{
				FieldSpec &s = m_fields[i];
				// if its a button, set the send field so 
				// lua knows which button was pressed
				if (s.is_button && s.fid == event.GUIEvent.Caller->getID())
				{
					s.send = true;
					acceptInput();
					if(s.is_exit){
						quitMenu();
						return true;
					}else{
						s.send = false;
						// Restore focus to the full form
						Environment->setFocus(this);
						return true;
					}
				}
			}
		}
		if(event.GUIEvent.EventType==gui::EGET_EDITBOX_ENTER)
		{
			if(event.GUIEvent.Caller->getID() > 257)
			{
				acceptInput();
				quitMenu();
				// quitMenu deallocates menu
				return true;
			}
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}

