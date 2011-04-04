/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/


#include "guiInventoryMenu.h"
#include "constants.h"

void drawInventoryItem(video::IVideoDriver *driver,
		gui::IGUIFont *font,
		InventoryItem *item, core::rect<s32> rect,
		const core::rect<s32> *clip)
{
	if(item == NULL)
		return;
	
	video::ITexture *texture = NULL;
	texture = item->getImage();

	if(texture != NULL)
	{
		const video::SColor color(255,255,255,255);
		const video::SColor colors[] = {color,color,color,color};
		driver->draw2DImage(texture, rect,
			core::rect<s32>(core::position2d<s32>(0,0),
			core::dimension2di(texture->getOriginalSize())),
			clip, colors, false);
	}
	else
	{
		video::SColor bgcolor(255,50,50,128);
		driver->draw2DRectangle(bgcolor, rect, clip);
	}

	if(font != NULL)
	{
		std::string text = item->getText();
		if(font && text != "")
		{
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
			
			font->draw(text.c_str(), rect2,
					video::SColor(255,255,255,255), false, false,
					clip);
		}
	}
}

/*
	GUIInventoryMenu
*/

GUIInventoryMenu::GUIInventoryMenu(gui::IGUIEnvironment* env,
		gui::IGUIElement* parent, s32 id,
		IMenuManager *menumgr,
		v2s16 menu_size,
		InventoryContext *c,
		InventoryManager *invmgr
		):
	GUIModalMenu(env, parent, id, menumgr),
	m_menu_size(menu_size),
	m_c(c),
	m_invmgr(invmgr)
{
	m_selected_item = NULL;
}

GUIInventoryMenu::~GUIInventoryMenu()
{
	removeChildren();

	if(m_selected_item)
		delete m_selected_item;
}

void GUIInventoryMenu::removeChildren()
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
}

void GUIInventoryMenu::regenerateGui(v2u32 screensize)
{
	// Remove children
	removeChildren();
	
	/*padding = v2s32(24,24);
	spacing = v2s32(60,56);
	imgsize = v2s32(48,48);*/

	padding = v2s32(screensize.X/48, screensize.X/48);
	spacing = v2s32(screensize.X/16, screensize.X/17);
	imgsize = v2s32(screensize.X/20, screensize.X/20);

	s32 helptext_h = 15;

	v2s32 size(
		padding.X*2+spacing.X*(m_menu_size.X-1)+imgsize.X,
		padding.Y*2+spacing.Y*(m_menu_size.Y-1)+imgsize.Y + helptext_h
	);

	core::rect<s32> rect(
			screensize.X/2 - size.X/2,
			screensize.Y/2 - size.Y/2,
			screensize.X/2 + size.X/2,
			screensize.Y/2 + size.Y/2
	);
	
	DesiredRect = rect;
	recalculateAbsolutePosition(false);

	v2s32 basepos = getBasePos();
	
	m_draw_spec.clear();
	for(u16 i=0; i<m_init_draw_spec.size(); i++)
	{
		DrawSpec &s = m_init_draw_spec[i];
		if(s.type == "list")
		{
			m_draw_spec.push_back(ListDrawSpec(s.name, s.subname,
					basepos + v2s32(spacing.X*s.pos.X, spacing.Y*s.pos.Y),
					s.geom));
		}
	}

	/*
	m_draw_spec.clear();
	m_draw_spec.push_back(ListDrawSpec("main",
			basepos + v2s32(spacing.X*0, spacing.Y*3), v2s32(8, 4)));
	m_draw_spec.push_back(ListDrawSpec("craft",
			basepos + v2s32(spacing.X*3, spacing.Y*0), v2s32(3, 3)));
	m_draw_spec.push_back(ListDrawSpec("craftresult",
			basepos + v2s32(spacing.X*7, spacing.Y*1), v2s32(1, 1)));
	*/
	
	// Add children
	{
		core::rect<s32> rect(0, 0, size.X-padding.X*2, helptext_h);
		rect = rect + v2s32(size.X/2 - rect.getWidth()/2,
				size.Y-rect.getHeight()-15);
		const wchar_t *text =
		L"Left click: Move all items, Right click: Move single item";
		Environment->addStaticText(text, rect, false, true, this, 256);
	}
}

GUIInventoryMenu::ItemSpec GUIInventoryMenu::getItemAtPos(v2s32 p) const
{
	core::rect<s32> imgrect(0,0,imgsize.X,imgsize.Y);
	
	for(u32 i=0; i<m_draw_spec.size(); i++)
	{
		const ListDrawSpec &s = m_draw_spec[i];

		for(s32 i=0; i<s.geom.X*s.geom.Y; i++)
		{
			s32 x = (i%s.geom.X) * spacing.X;
			s32 y = (i/s.geom.X) * spacing.Y;
			v2s32 p0(x,y);
			core::rect<s32> rect = imgrect + s.pos + p0;
			if(rect.isPointInside(p))
			{
				return ItemSpec(s.inventoryname, s.listname, i);
			}
		}
	}

	return ItemSpec("", "", -1);
}

void GUIInventoryMenu::drawList(const ListDrawSpec &s)
{
	video::IVideoDriver* driver = Environment->getVideoDriver();

	// Get font
	gui::IGUIFont *font = NULL;
	gui::IGUISkin* skin = Environment->getSkin();
	if (skin)
		font = skin->getFont();
	
	Inventory *inv = m_invmgr->getInventory(m_c, s.inventoryname);
	assert(inv);
	InventoryList *ilist = inv->getList(s.listname);
	
	core::rect<s32> imgrect(0,0,imgsize.X,imgsize.Y);
	
	for(s32 i=0; i<s.geom.X*s.geom.Y; i++)
	{
		s32 x = (i%s.geom.X) * spacing.X;
		s32 y = (i/s.geom.X) * spacing.Y;
		v2s32 p(x,y);
		core::rect<s32> rect = imgrect + s.pos + p;
		InventoryItem *item = NULL;
		if(ilist)
			item = ilist->getItem(i);

		if(m_selected_item != NULL && m_selected_item->listname == s.listname
				&& m_selected_item->i == i)
		{
			driver->draw2DRectangle(video::SColor(255,255,0,0),
					core::rect<s32>(rect.UpperLeftCorner - v2s32(2,2),
							rect.LowerRightCorner + v2s32(2,2)),
					&AbsoluteClippingRect);
		}

		if(item)
		{
			drawInventoryItem(driver, font, item,
					rect, &AbsoluteClippingRect);
		}
		else
		{
			video::SColor bgcolor(255,128,128,128);
			driver->draw2DRectangle(bgcolor, rect, &AbsoluteClippingRect);
		}
	}
}

void GUIInventoryMenu::drawMenu()
{
	gui::IGUISkin* skin = Environment->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = Environment->getVideoDriver();
	
	video::SColor bgcolor(140,0,0,0);
	driver->draw2DRectangle(bgcolor, AbsoluteRect, &AbsoluteClippingRect);

	/*
		Draw items
	*/
	
	for(u32 i=0; i<m_draw_spec.size(); i++)
	{
		ListDrawSpec &s = m_draw_spec[i];
		drawList(s);
	}

	/*
		Call base class
	*/
	gui::IGUIElement::draw();
}

bool GUIInventoryMenu::OnEvent(const SEvent& event)
{
	if(event.EventType==EET_KEY_INPUT_EVENT)
	{
		if(event.KeyInput.Key==KEY_ESCAPE && event.KeyInput.PressedDown)
		{
			quitMenu();
			return true;
		}
		if(event.KeyInput.Key==KEY_KEY_I && event.KeyInput.PressedDown)
		{
			quitMenu();
			return true;
		}
	}
	if(event.EventType==EET_MOUSE_INPUT_EVENT)
	{
		if(event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN
				|| event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN)
		{
			bool right = (event.MouseInput.Event == EMIE_RMOUSE_PRESSED_DOWN);
			v2s32 p(event.MouseInput.X, event.MouseInput.Y);
			//dstream<<"Mouse down at p=("<<p.X<<","<<p.Y<<")"<<std::endl;
			ItemSpec s = getItemAtPos(p);
			if(s.isValid())
			{
				dstream<<"Mouse down on "<<s.inventoryname
						<<"/"<<s.listname<<" "<<s.i<<std::endl;
				if(m_selected_item)
				{
					Inventory *inv_from = m_invmgr->getInventory(m_c,
							m_selected_item->inventoryname);
					Inventory *inv_to = m_invmgr->getInventory(m_c,
							s.inventoryname);
					assert(inv_from);
					assert(inv_to);
					InventoryList *list_from =
							inv_from->getList(m_selected_item->listname);
					InventoryList *list_to =
							inv_to->getList(s.listname);
					if(list_from == NULL)
						dstream<<"from list doesn't exist"<<std::endl;
					if(list_to == NULL)
						dstream<<"to list doesn't exist"<<std::endl;
					// Indicates whether source slot completely empties
					bool source_empties = false;
					if(list_from && list_to
							&& list_from->getItem(m_selected_item->i) != NULL)
					{
						dstream<<"Handing IACTION_MOVE to manager"<<std::endl;
						IMoveAction *a = new IMoveAction();
						a->count = right ? 1 : 0;
						a->from_inv = m_selected_item->inventoryname;
						a->from_list = m_selected_item->listname;
						a->from_i = m_selected_item->i;
						a->to_inv = s.inventoryname;
						a->to_list = s.listname;
						a->to_i = s.i;
						//ispec.actions->push_back(a);
						m_invmgr->inventoryAction(a);
						
						if(list_from->getItem(m_selected_item->i)->getCount()==1)
							source_empties = true;
					}
					// Remove selection if target was left-clicked or source
					// slot was emptied
					if(right == false || source_empties)
					{
						delete m_selected_item;
						m_selected_item = NULL;
					}
				}
				else
				{
					/*
						Select if non-NULL
					*/
					Inventory *inv = m_invmgr->getInventory(m_c,
							s.inventoryname);
					assert(inv);
					InventoryList *list = inv->getList(s.listname);
					if(list->getItem(s.i) != NULL)
					{
						m_selected_item = new ItemSpec(s);
					}
				}
			}
			else
			{
				if(m_selected_item)
				{
					delete m_selected_item;
					m_selected_item = NULL;
				}
			}
		}
	}
	if(event.EventType==EET_GUI_EVENT)
	{
		if(event.GUIEvent.EventType==gui::EGET_ELEMENT_FOCUS_LOST
				&& isVisible())
		{
			if(!canTakeFocus(event.GUIEvent.Element))
			{
				dstream<<"GUIInventoryMenu: Not allowing focus change."
						<<std::endl;
				// Returning true disables focus change
				return true;
			}
		}
		if(event.GUIEvent.EventType==gui::EGET_BUTTON_CLICKED)
		{
			/*switch(event.GUIEvent.Caller->getID())
			{
			case 256: // continue
				setVisible(false);
				break;
			case 257: // exit
				dev->closeDevice();
				break;
			}*/
		}
	}

	return Parent ? Parent->OnEvent(event) : false;
}



