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

void drawInventoryItem(gui::IGUIEnvironment* env,
		InventoryItem *item, core::rect<s32> rect,
		const core::rect<s32> *clip)
{
	gui::IGUISkin* skin = env->getSkin();
	if (!skin)
		return;
	video::IVideoDriver* driver = env->getVideoDriver();
	
	video::ITexture *texture = NULL;
	
	if(item != NULL)
	{
		texture = item->getImage();
	}

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
		video::SColor bgcolor(128,128,128,128);
		driver->draw2DRectangle(bgcolor, rect, clip);
	}

	if(item != NULL)
	{
		gui::IGUIFont *font = skin->getFont();
		std::string text = item->getText();
		if(font && text != "")
		{
			core::rect<s32> rect2(rect.UpperLeftCorner,
					(core::dimension2d<u32>(rect.getWidth(), 15)));

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
		Inventory *inventory,
		Queue<InventoryAction*> *actions,
		IMenuManager *menumgr):
	GUIModalMenu(env, parent, id, menumgr)
{
	m_inventory = inventory;
	m_selected_item = NULL;
	m_actions = actions;

	/*m_selected_item = new ItemSpec;
	m_selected_item->listname = "main";
	m_selected_item->i = 3;*/
}

GUIInventoryMenu::~GUIInventoryMenu()
{
	removeChildren();

	if(m_selected_item)
		delete m_selected_item;
}

void GUIInventoryMenu::removeChildren()
{
	{
		gui::IGUIElement *e = getElementFromId(256);
		if(e != NULL)
			e->remove();
	}
}

void GUIInventoryMenu::regenerateGui(v2u32 screensize)
{
	// Remove children
	removeChildren();
	
	padding = v2s32(24,24);
	spacing = v2s32(60,56);
	imgsize = v2s32(48,48);

	s32 helptext_h = 15;

	v2s32 size(
		padding.X*2+spacing.X*(8-1)+imgsize.X,
		padding.Y*2+spacing.Y*(7-1)+imgsize.Y + helptext_h
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
	
	m_draw_positions.clear();
	m_draw_positions.push_back(ListDrawSpec("main",
			basepos + v2s32(spacing.X*0, spacing.Y*3), v2s32(8, 4)));
	m_draw_positions.push_back(ListDrawSpec("craft",
			basepos + v2s32(spacing.X*3, spacing.Y*0), v2s32(3, 3)));
	m_draw_positions.push_back(ListDrawSpec("craftresult",
			basepos + v2s32(spacing.X*7, spacing.Y*1), v2s32(1, 1)));
	
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
	
	for(u32 i=0; i<m_draw_positions.size(); i++)
	{
		const ListDrawSpec &s = m_draw_positions[i];

		for(s32 i=0; i<s.geom.X*s.geom.Y; i++)
		{
			s32 x = (i%s.geom.X) * spacing.X;
			s32 y = (i/s.geom.X) * spacing.Y;
			v2s32 p0(x,y);
			core::rect<s32> rect = imgrect + s.pos + p0;
			if(rect.isPointInside(p))
			{
				return ItemSpec(s.listname, i);
			}
		}
	}

	return ItemSpec("", -1);
}

//void GUIInventoryMenu::drawList(const std::string &name, v2s32 pos, v2s32 geom)
void GUIInventoryMenu::drawList(const ListDrawSpec &s)
{
	video::IVideoDriver* driver = Environment->getVideoDriver();

	InventoryList *ilist = m_inventory->getList(s.listname);
	
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
		drawInventoryItem(Environment, item,
				rect, &AbsoluteClippingRect);
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
	
	for(u32 i=0; i<m_draw_positions.size(); i++)
	{
		ListDrawSpec &s = m_draw_positions[i];
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
				//dstream<<"Mouse down on "<<s.listname<<" "<<s.i<<std::endl;
				if(m_selected_item)
				{
					InventoryList *list_from =
							m_inventory->getList(m_selected_item->listname);
					InventoryList *list_to =
							m_inventory->getList(s.listname);
					// Indicates whether source slot completely empties
					bool source_empties = false;
					if(list_from && list_to
							&& list_from->getItem(m_selected_item->i) != NULL)
					{
						dstream<<"Queueing IACTION_MOVE"<<std::endl;
						IMoveAction *a = new IMoveAction();
						a->count = right ? 1 : 0;
						a->from_name = m_selected_item->listname;
						a->from_i = m_selected_item->i;
						a->to_name = s.listname;
						a->to_i = s.i;
						m_actions->push_back(a);
						
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
					InventoryList *list = m_inventory->getList(s.listname);
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



