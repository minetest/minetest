#include "guiImageTabControl.h"

namespace irr
{
namespace gui
{
//! constructor
guiImageTab::guiImageTab(s32 number, IGUIEnvironment* environment,
	IGUIElement* parent, const core::rect<s32>& rectangle,
	s32 id, video::ITexture *texture, f32 scaling, u32 side)
	: IGUITab(environment, parent, id, rectangle), Number(number),
		BackColor(0,0,0,0), OverrideTextColorEnabled(false), TextColor(255,0,0,0),
		DrawBackground(false), 
		Texture(texture), Scaling(scaling), Side(side), Active(false), Drawn(false),
		DrawnRect(rectangle)
{
	#ifdef _DEBUG
	setDebugName("guiImageTab");
	#endif

	const IGUISkin* const skin = environment->getSkin();
	if (skin)
		TextColor = skin->getColor(EGDC_BUTTON_TEXT);
}


//! Returns number of tab in tabcontrol. Can be accessed
//! later IGUITabControl::getTab() by this number.
s32 guiImageTab::getNumber() const
{
	return Number;
}


//! Sets the number
void guiImageTab::setNumber(s32 n)
{
	Number = n;
}

void guiImageTab::refreshSkinColors()
{
	if ( !OverrideTextColorEnabled )
	{
		TextColor = Environment->getSkin()->getColor(EGDC_BUTTON_TEXT);
	}
}

//! draws the element and its children
void guiImageTab::draw()
{
	if (!IsVisible)
		return;

	IGUIElement::draw();
}


//! sets if the tab should draw its background
void guiImageTab::setDrawBackground(bool draw)
{
	DrawBackground = draw;
}


//! sets the color of the background, if it should be drawn.
void guiImageTab::setBackgroundColor(video::SColor c)
{
	BackColor = c;
}


//! sets the color of the text
void guiImageTab::setTextColor(video::SColor c)
{
	OverrideTextColorEnabled = true;
	TextColor = c;
}


video::SColor guiImageTab::getTextColor() const
{
	return TextColor;
}


//! returns true if the tab is drawing its background, false if not
bool guiImageTab::isDrawingBackground() const
{
	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return DrawBackground;
}


//! returns the color of the background
video::SColor guiImageTab::getBackgroundColor() const
{
	return BackColor;
}


//! Writes attributes of the element.
void guiImageTab::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const
{
	IGUITab::serializeAttributes(out,options);

	out->addInt		("TabNumber",		Number);
	out->addBool	("DrawBackground",	DrawBackground);
	out->addColor	("BackColor",		BackColor);
	out->addBool	("OverrideTextColorEnabled", OverrideTextColorEnabled);
	out->addColor	("TextColor",		TextColor);

}


//! Reads attributes of the element
void guiImageTab::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0)
{
	IGUITab::deserializeAttributes(in,options);

	setNumber(in->getAttributeAsInt("TabNumber"));
	setDrawBackground(in->getAttributeAsBool("DrawBackground"));
	setBackgroundColor(in->getAttributeAsColor("BackColor"));
	bool override = in->getAttributeAsBool("OverrideTextColorEnabled");
	setTextColor(in->getAttributeAsColor("TextColor"));
	if ( !override )
	{
		OverrideTextColorEnabled = false;
	}

	if (Parent && Parent->getType() == EGUIET_TAB_CONTROL)
	{
		((guiImageTabControl*)Parent)->addTab(this);
		if (isVisible())
			((guiImageTabControl*)Parent)->setActiveTab(this);
	}
}


//! Draws the tab image
void guiImageTab::drawImage(
	const irr::core::rect<s32>& tabRect
	)
{
	if (Texture)
	{
		f32 margin = 4;
		
		f32 max_width = ( tabRect.LowerRightCorner.X - tabRect.UpperLeftCorner.X - 2 * margin ) * Scaling;
		f32 max_height = ( tabRect.LowerRightCorner.Y - tabRect.UpperLeftCorner.Y - 2 * margin ) * Scaling;
		
		f32 tab_height = max_height;
		f32 tab_width = tab_height * Texture->getSize().Width / Texture->getSize().Height;
		
		if ( tab_width > max_width )
		{
			tab_height *= max_width / tab_width;
			tab_width = max_width;
		}
		
		f32 middle_x = ( tabRect.LowerRightCorner.X + tabRect.UpperLeftCorner.X ) * 0.5f;
		f32 middle_y = ( tabRect.LowerRightCorner.Y + tabRect.UpperLeftCorner.Y ) * 0.5f;
		
		video::IVideoDriver* driver = Environment->getVideoDriver();

		driver->draw2DImage(Texture,
			irr::core::rect<s32>(middle_x - tab_width * 0.5f, middle_y - tab_height * 0.5f, 
				middle_x + tab_width * 0.5f, middle_y + tab_height * 0.5f ), 
			irr::core::rect<s32>(0, 0, Texture->getSize().Width, Texture->getSize().Height), 
			0, 0, true);
	}
}

// ------------------------------------------------------------------
// Tabcontrol
// ------------------------------------------------------------------

//! constructor
guiImageTabControl::guiImageTabControl(IGUIEnvironment* environment,
	IGUIElement* parent, const core::rect<s32>& rectangle, s32 id, 
	s32 tab_height, s32 tab_width, s32 tab_padding, s32 tab_spacing, 
	s32 width, s32 height, s32 border_width, s32 border_height, s32 border_offset,
	s32 button_width, s32 button_height, s32 button_spacing, s32 button_offset, s32 button_distance, 
	video::ITexture* content_texture, 
	video::ITexture* top_tab_texture, video::ITexture* top_active_tab_texture,  
	video::ITexture* bottom_tab_texture, video::ITexture* bottom_active_tab_texture,  
	video::ITexture* left_tab_texture, video::ITexture* left_active_tab_texture,  
	video::ITexture* right_tab_texture, video::ITexture* right_active_tab_texture,  
	video::ITexture* up_arrow_texture, video::ITexture* up_arrow_pressed_texture,
	video::ITexture* down_arrow_texture, video::ITexture* down_arrow_pressed_texture,
	video::ITexture* left_arrow_texture, video::ITexture* left_arrow_pressed_texture,
	video::ITexture* right_arrow_texture, video::ITexture* right_arrow_pressed_texture)
	: IGUITabControl(environment, parent, id, rectangle),  
	TabHeight(tab_height), TabWidth(tab_width), 
	TabPadding(tab_padding), TabSpacing(tab_spacing),
	Width(width), Height(height), BorderWidth(border_width), BorderHeight(border_height), BorderOffset(border_offset),
	ButtonWidth(button_width), ButtonHeight(button_height), 
	ButtonSpacing(button_spacing), ButtonOffset(button_offset), ButtonDistance(button_distance), 
	VerticalAlignment(EGUIA_UPPERLEFT),	ActiveTabIndex(-1), Skin(0), ContentTexture(content_texture), ContentRect(0, 0, 0, 0)
{
	#ifdef _DEBUG
	setDebugName("guiImageTabControl");
	#endif
	
	Skin = environment->getSkin();
	
	for (s32 side=0; side < 4; ++side) 
	{
		SideScrollControl[side] = false;
		SidePriorArrow[side] = 0;
		SideNextArrow[side] = 0;
		SideFirstScrollTabIndex[side] = 0;
		SideLastScrollTabIndex[side] = -1;
	}

	SideTabTexture[0] = top_tab_texture;
	SideActiveTabTexture[0] = top_active_tab_texture;
	SidePriorArrowTexture[0] = left_arrow_texture;
	SidePriorArrowPressedTexture[0] = left_arrow_pressed_texture;
	SideNextArrowTexture[0] = right_arrow_texture;
	SideNextArrowPressedTexture[0] = right_arrow_pressed_texture;

	SideTabTexture[1] = bottom_tab_texture;
	SideActiveTabTexture[1] = bottom_active_tab_texture;
	SidePriorArrowTexture[1] = left_arrow_texture;
	SidePriorArrowPressedTexture[1] = left_arrow_pressed_texture;
	SideNextArrowTexture[1] = right_arrow_texture;
	SideNextArrowPressedTexture[1] = right_arrow_pressed_texture;

	SideTabTexture[2] = left_tab_texture;
	SideActiveTabTexture[2] = left_active_tab_texture;
	SidePriorArrowTexture[2] = up_arrow_texture;
	SidePriorArrowPressedTexture[2] = up_arrow_pressed_texture;
	SideNextArrowTexture[2] = down_arrow_texture;
	SideNextArrowPressedTexture[2] = down_arrow_pressed_texture;

	SideTabTexture[3] = right_tab_texture;
	SideActiveTabTexture[3] = right_active_tab_texture;
	SidePriorArrowTexture[3] = up_arrow_texture;
	SidePriorArrowPressedTexture[3] = up_arrow_pressed_texture;
	SideNextArrowTexture[3] = down_arrow_texture;
	SideNextArrowPressedTexture[3] = down_arrow_pressed_texture;

	for (s32 side=0; side < 4; ++side) 
	{
		SidePriorArrow[side] = Environment->addButton(core::rect<s32>(0,0,10,10), this);

		if ( SidePriorArrow[side] )
		{
			SidePriorArrow[side]->setImage(SidePriorArrowTexture[side]);
			SidePriorArrow[side]->setPressedImage(SidePriorArrowPressedTexture[side]);
			SidePriorArrow[side]->setDrawBorder(false);
			SidePriorArrow[side]->setScaleImage(true);
			SidePriorArrow[side]->setUseAlphaChannel(true);
			SidePriorArrow[side]->setVisible(false);
			SidePriorArrow[side]->setSubElement(true);
			SidePriorArrow[side]->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
			SidePriorArrow[side]->setOverrideFont(Environment->getBuiltInFont());
			SidePriorArrow[side]->grab();
		}

		SideNextArrow[side] = Environment->addButton(core::rect<s32>(0,0,10,10), this);

		if ( SideNextArrow[side] )
		{
			SideNextArrow[side]->setImage(SideNextArrowTexture[side]);
			SideNextArrow[side]->setPressedImage(SideNextArrowPressedTexture[side]);
			SideNextArrow[side]->setDrawBorder(false);
			SideNextArrow[side]->setScaleImage(true);
			SideNextArrow[side]->setUseAlphaChannel(true);
			SideNextArrow[side]->setVisible(false);
			SideNextArrow[side]->setSubElement(true);
			SideNextArrow[side]->setAlignment(EGUIA_LOWERRIGHT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_UPPERLEFT);
			SideNextArrow[side]->setOverrideFont(Environment->getBuiltInFont());
			SideNextArrow[side]->grab();
		}
	}
	
	setTabVerticalAlignment(EGUIA_UPPERLEFT);
	refreshSprites();
}

//! destructor
guiImageTabControl::~guiImageTabControl()
{
	for (u32 i=0; i<Tabs.size(); ++i)
	{
		if (Tabs[i])
			Tabs[i]->drop();
	}

	for (u32 side=0; side < 4; ++side)
	{
		if (SidePriorArrow[side])
			SidePriorArrow[side]->drop();

		if (SideNextArrow[side])
			SideNextArrow[side]->drop();
	}
}

void guiImageTabControl::refreshSprites()
{
	video::SColor color(255,255,255,255);
	IGUISkin* skin = Environment->getSkin();
	
	if (skin)
	{
		color = skin->getColor(isEnabled() ? EGDC_WINDOW_SYMBOL : EGDC_GRAY_WINDOW_SYMBOL);
	}

	for (u32 side=0; side < 4; ++side)
	{
		if (SidePriorArrow[side])
		{
			SidePriorArrow[side]->setSprite(EGBS_BUTTON_UP, skin->getIcon(EGDI_CURSOR_LEFT), color);
			SidePriorArrow[side]->setSprite(EGBS_BUTTON_DOWN, skin->getIcon(EGDI_CURSOR_LEFT), color);
		}

		if (SideNextArrow[side])
		{
			SideNextArrow[side]->setSprite(EGBS_BUTTON_UP, skin->getIcon(EGDI_CURSOR_RIGHT), color);
			SideNextArrow[side]->setSprite(EGBS_BUTTON_DOWN, skin->getIcon(EGDI_CURSOR_RIGHT), color);
		}
	}
}

//! Adds a tab
IGUITab* guiImageTabControl::addTab(const wchar_t* caption, s32 id)
{
	return addImageTab(caption, id, 0);
}

//! Adds an image tab
guiImageTab* guiImageTabControl::addImageTab(const wchar_t* caption, s32 id, 
	video::ITexture *texture, f32 scaling, u32 side)
{
	guiImageTab* tab = new guiImageTab(Tabs.size(), Environment, this, calcRelativeRect(), id, 
		texture, scaling, side);

	if (!texture)
	{
		tab->setText(caption);
	}
	
	tab->setAlignment(EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT);
	tab->setVisible(false);
	
	Tabs.push_back(tab);
	SideTabs[side].push_back(tab);

	if (ActiveTabIndex == -1)
	{
		ActiveTabIndex = 0;
		tab->setVisible(true);
	}

	return tab;
}


//! adds a tab which has been created elsewhere
void guiImageTabControl::addTab(guiImageTab* tab)
{
	if (!tab)
		return;

	// check if its already added
	for (u32 i=0; i < Tabs.size(); ++i)
	{
		if (Tabs[i] == tab)
			return;
	}

	tab->grab();

	if (tab->getNumber() == -1)
		tab->setNumber((s32)Tabs.size());

	while (tab->getNumber() >= (s32)Tabs.size())
		Tabs.push_back(0);

	if (Tabs[tab->getNumber()])
	{
		Tabs.push_back(Tabs[tab->getNumber()]);
		Tabs[Tabs.size()-1]->setNumber(Tabs.size());
	}
	
	Tabs[tab->getNumber()] = tab;
	
	SideTabs[tab->Side].push_back(tab);

	if (ActiveTabIndex == -1)
		ActiveTabIndex = tab->getNumber();

	if (tab->getNumber() == ActiveTabIndex)
	{
		setActiveTab(ActiveTabIndex);
	}
}

//! Insert the tab at the given index
IGUITab* guiImageTabControl::insertTab(s32 idx, const wchar_t* caption, s32 id)
{
	if ( idx < 0 || idx > (s32)Tabs.size() )
		return NULL;

	guiImageTab* tab = new guiImageTab(idx, Environment, this, calcRelativeRect(), id);

	tab->setText(caption);
	tab->setAlignment(EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT, EGUIA_UPPERLEFT, EGUIA_LOWERRIGHT);
	tab->setVisible(false);
	Tabs.insert(tab, (u32)idx);
	
	SideTabs[tab->Side].push_back(tab);

	if (ActiveTabIndex == -1)
	{
		ActiveTabIndex = 0;
		tab->setVisible(true);
	}

	for ( u32 i=(u32)idx+1; i < Tabs.size(); ++i )
	{
		Tabs[i]->setNumber(i);
	}

	return tab;
}

//! Removes a tab from the tabcontrol
void guiImageTabControl::removeTab(s32 idx)
{
	if ( idx < 0 || idx >= (s32)Tabs.size() )
		return;
		
	guiImageTab* tab = Tabs[(u32)idx];
	
	for (u32 i=0; i < SideTabs[tab->Side].size(); ++i)
	{
		if (SideTabs[tab->Side][i] == tab) 
		{
			SideTabs[tab->Side].erase(i);
			break;
		}
	}

	tab->drop();
	Tabs.erase((u32)idx);
	
	for ( u32 i=(u32)idx; i < Tabs.size(); ++i )
	{
		Tabs[i]->setNumber(i);
	}
}

//! Clears the tabcontrol removing all tabs
void guiImageTabControl::clear()
{
	for (u32 i=0; i<Tabs.size(); ++i)
	{
		if (Tabs[i])
			Tabs[i]->drop();
	}
	Tabs.clear();
	
	for (u32 side=0; side < 4; ++side)
		SideTabs[side].clear();
}

//! Returns amount of tabs in the tabcontrol
s32 guiImageTabControl::getTabCount() const
{
	return Tabs.size();
}


//! Returns a tab based on zero based index
IGUITab* guiImageTabControl::getTab(s32 idx) const
{
	if ((u32)idx >= Tabs.size())
		return 0;

	return Tabs[idx];
}


//! called if an event happened.
bool guiImageTabControl::OnEvent(const SEvent& event)
{
	if (isEnabled())
	{

		switch(event.EventType)
		{
		case EET_GUI_EVENT:
			switch(event.GUIEvent.EventType)
			{
			case EGET_BUTTON_CLICKED:
			{
				for (u32 side=0; side < 4; ++side)
				{
					if (event.GUIEvent.Caller == SidePriorArrow[side])
					{
						scrollLeft(side);
						return true;
					}
					else if (event.GUIEvent.Caller == SideNextArrow[side])
					{
						scrollRight(side);
						return true;
					}
				}
				break;
			}
			default:
			break;
			}
			break;
		case EET_MOUSE_INPUT_EVENT:
			switch(event.MouseInput.Event)
			{
			case EMIE_LMOUSE_PRESSED_DOWN:
				break;
			case EMIE_LMOUSE_LEFT_UP:
			{
				s32 idx = getTabAt(event.MouseInput.X, event.MouseInput.Y);
				if ( idx >= 0 )
				{
					setActiveTab(idx);
					return true;
				}
				break;
			}
			case EMIE_MOUSE_WHEEL:
			{
				s32 idx = getTabAt(event.MouseInput.X, event.MouseInput.Y);
				if ( idx >= 0 )
				{
					s32 new_tab_index = ActiveTabIndex;
					
					if ( event.MouseInput.Wheel < 0 )
						++new_tab_index;
					else if ( event.MouseInput.Wheel > 0 )
						--new_tab_index;
						
					if ( new_tab_index < 0 )
						new_tab_index = Tabs.size() - 1;
					else if ( new_tab_index >= (s32)Tabs.size() )
						new_tab_index = 0;
					
					if ( new_tab_index != ActiveTabIndex
					     && new_tab_index >= 0 
					     && new_tab_index < (s32)Tabs.size() )
					{
						setActiveTab(new_tab_index);
						guiImageTab* tab = Tabs[new_tab_index];
						u32 side = tab->Side;
	
						if ( SideScrollControl[side] )
						{
							for (s32 side_tab_index=0; side_tab_index < (s32)SideTabs[side].size(); ++side_tab_index)
							{
								if ( SideTabs[side][side_tab_index] == tab )
								{
									if ( side_tab_index < SideFirstScrollTabIndex[side]
									     || side_tab_index > SideLastScrollTabIndex[side] )
									{
										SideFirstScrollTabIndex[side] = side_tab_index;
										SideLastScrollTabIndex[side] = side_tab_index;
									}
								}
							}
						}
						return true;
					}
				}
				break;
			}
			default:
				break;
			}
			break;
		default:
			break;
		}
	}

	return IGUIElement::OnEvent(event);
}


void guiImageTabControl::scrollLeft(u32 side)
{
	if ( SideScrollControl[side]
	     && SideFirstScrollTabIndex[side] > 0 )
	{
		--SideFirstScrollTabIndex[side];
	}
}


void guiImageTabControl::scrollRight(u32 side)
{
	if ( SideScrollControl[side]
		 && SideFirstScrollTabIndex[side] < (s32)(SideTabs[side].size()) - 1 )
	{
		++SideFirstScrollTabIndex[side];
	}
}


s32 guiImageTabControl::calcTabWidth(s32 pos, IGUIFont* font, const wchar_t* text, bool withScrollControl,
	guiImageTab* tab) const
{
	if ( !font )
		return 0;

	if ( tab->Side >= 2 )
		return TabWidth;

	s32 len = font->getDimension(text).Width + TabPadding;
	
	if ( tab->Texture )
	{
		len = TabHeight * tab->Scaling * tab->Texture->getSize().Width / tab->Texture->getSize().Height + TabPadding;
	}
	
	return len;
}


void guiImageTabControl::calcTabs()
{	
	if ( !IsVisible )
		return;

	IGUISkin* skin = Environment->getSkin();
	
	if ( !skin )
		return;

	IGUIFont* font = skin->getFont();
	
	if ( !font )
		return;
		
	guiImageTab* tab;
	
	for (u32 i=0; i<Tabs.size(); ++i)
	{
		tab = Tabs[i];
		
		if ( tab )
		{
			tab->Active = false;
			tab->Drawn = false;
		}
	}
	
	for (u32 side=0; side < 4; ++side)
	{
		if ( SideFirstScrollTabIndex[side] >= (s32)SideTabs[side].size() )
			SideFirstScrollTabIndex[side] = ((s32)SideTabs[side].size()) - 1;

		if ( SideFirstScrollTabIndex[side] < 0 )
			SideFirstScrollTabIndex[side] = 0;
			
		s32 pos;

		if ( side < 2 )
		{
			pos = AbsoluteRect.UpperLeftCorner.X + TabWidth + BorderWidth;
		}
		else
		{
			pos = AbsoluteRect.UpperLeftCorner.Y + TabHeight + BorderHeight;
		}
		
		core::rect<s32> tabRect;
		
		SideLastScrollTabIndex[side] = -1;

		for (u32 i=SideFirstScrollTabIndex[side]; i<SideTabs[side].size(); ++i)
		{
			tab = SideTabs[side][i];
			
			if ( tab )
			{
				const wchar_t* text = 0;
			
				text = SideTabs[side][i]->getText();

				// get text length
				s32 len = calcTabWidth(pos, font, text, true, tab);
						
				if ( side < 2 )
				{
					tabRect.UpperLeftCorner.X = pos;
					pos += len + TabSpacing;
					
					if ( SideScrollControl[side]
						 && pos > AbsoluteRect.LowerRightCorner.X - TabWidth - ButtonOffset - 2 * ( ButtonWidth + ButtonSpacing ) - BorderWidth )
					{
						break;		
					}				
					
					if ( pos > AbsoluteRect.LowerRightCorner.X - TabWidth - BorderWidth )
					{
						SideScrollControl[side] = true;	
						break;		
					}				
				}
				else
				{
					tabRect.UpperLeftCorner.Y = pos;			
					pos += TabHeight + TabSpacing;
					
					if ( SideScrollControl[side]
						 && pos > AbsoluteRect.LowerRightCorner.Y - TabHeight - ButtonOffset - 2 * ( ButtonHeight + ButtonSpacing ) - BorderHeight )
					{
						break;		
					}				
					
					if ( pos > AbsoluteRect.LowerRightCorner.Y - TabHeight - BorderHeight )
					{			
						SideScrollControl[side] = true;
						break;		
					}
				}

				if ( side == 0 )
				{
					tabRect.UpperLeftCorner.Y = AbsoluteRect.UpperLeftCorner.Y;
				}
				else if ( side == 1 )
				{
					tabRect.UpperLeftCorner.Y = AbsoluteRect.LowerRightCorner.Y - TabHeight;
				}
				else if ( side == 2 )
				{
					tabRect.UpperLeftCorner.X = AbsoluteRect.UpperLeftCorner.X;
				}
				else
				{
					tabRect.UpperLeftCorner.X = AbsoluteRect.LowerRightCorner.X - TabWidth;
				}
				
				tabRect.LowerRightCorner.X = tabRect.UpperLeftCorner.X + len;
				tabRect.LowerRightCorner.Y = tabRect.UpperLeftCorner.Y + TabHeight;

				if ( tab->Number == ActiveTabIndex )
				{
					tab->Active = true;
				}
				
				tab->Drawn = true;
				tab->DrawnRect = tabRect;
				
				if ( text )
					tab->refreshSkinColors();
					
				SideLastScrollTabIndex[side] = i;
			}
		}
	}
		
	ContentRect = AbsoluteRect;
	
	ContentRect.UpperLeftCorner.Y += TabHeight;
	ContentRect.LowerRightCorner.Y -= TabHeight;
	ContentRect.UpperLeftCorner.X += TabWidth;
	ContentRect.LowerRightCorner.X -= TabWidth;
}


void guiImageTabControl::calcScrollButtons()
{
	core::rect<s32> buttonRect;
	
	for (u32 side=0; side < 4; ++side)
	{
		if ( side < 2 )
		{
			buttonRect.UpperLeftCorner.X = AbsoluteRect.getWidth() - TabWidth - ButtonOffset - 2 * ButtonWidth - ButtonSpacing;
			
			if ( side == 0 )
			{
				buttonRect.UpperLeftCorner.Y = TabHeight - ButtonHeight - ButtonDistance;
			}
			else
			{
				buttonRect.UpperLeftCorner.Y = AbsoluteRect.getHeight() - TabHeight + ButtonDistance;
			}
			
			buttonRect.LowerRightCorner.X = buttonRect.UpperLeftCorner.X + ButtonWidth;
			buttonRect.LowerRightCorner.Y = buttonRect.UpperLeftCorner.Y + ButtonHeight;
			SidePriorArrow[side]->setRelativePosition(buttonRect);

			buttonRect.UpperLeftCorner.X += ButtonWidth + ButtonSpacing;
			
			buttonRect.LowerRightCorner.X = buttonRect.UpperLeftCorner.X + ButtonWidth;
			buttonRect.LowerRightCorner.Y = buttonRect.UpperLeftCorner.Y + ButtonHeight;
			SideNextArrow[side]->setRelativePosition(buttonRect);
		}
		else
		{
			buttonRect.UpperLeftCorner.Y = AbsoluteRect.getHeight() - TabHeight - ButtonOffset - 2 * ButtonHeight - ButtonSpacing;
			
			if ( side == 2 )
			{
				buttonRect.UpperLeftCorner.X = TabWidth - ButtonWidth - ButtonDistance;
			}
			else
			{
				buttonRect.UpperLeftCorner.X = AbsoluteRect.getWidth() - TabWidth + ButtonDistance;
			}
			
			buttonRect.LowerRightCorner.X = buttonRect.UpperLeftCorner.X + ButtonWidth;
			buttonRect.LowerRightCorner.Y = buttonRect.UpperLeftCorner.Y + ButtonHeight;
			SidePriorArrow[side]->setRelativePosition(buttonRect);

			buttonRect.UpperLeftCorner.Y += ButtonHeight + ButtonSpacing;
			
			buttonRect.LowerRightCorner.X = buttonRect.UpperLeftCorner.X + ButtonWidth;
			buttonRect.LowerRightCorner.Y = buttonRect.UpperLeftCorner.Y + ButtonHeight;
			SideNextArrow[side]->setRelativePosition(buttonRect);
		}
		
		if (SidePriorArrow[side] && SideNextArrow[side])
		{
			SidePriorArrow[side]->setVisible( SideScrollControl[side] );
			SideNextArrow[side]->setVisible( SideScrollControl[side] );

			bringToFront( SidePriorArrow[side] );
			bringToFront( SideNextArrow[side] );
		}
	}
}


//! Computes a tab position
core::rect<s32> guiImageTabControl::calcRelativeRect()
{
	core::rect<s32> r;
	
	r.UpperLeftCorner.X = 0;
	r.UpperLeftCorner.Y = 0;
	r.LowerRightCorner.X = AbsoluteRect.getWidth();	
	r.LowerRightCorner.Y = AbsoluteRect.getHeight();

	return r;
}

//! Draws a tab
void guiImageTabControl::drawTab(guiImageTab* tab, IGUIFont* font)
{
	core::rect<s32> tab_rect(tab->DrawnRect);				
	const wchar_t* text = tab->getText();
	u32 side = tab->Side;
	video::ITexture* tab_texture = tab->Active ? SideActiveTabTexture[side] : SideTabTexture[side];

	if ( side == 0 )
	{
		tab_rect.LowerRightCorner.Y += BorderOffset;
	}
	else if ( side == 1 )
	{
		tab_rect.UpperLeftCorner.Y -= BorderOffset;
	}
	else if ( side == 2 )
	{
		tab_rect.LowerRightCorner.X += BorderOffset;
	}
	else
	{
		tab_rect.UpperLeftCorner.X -= BorderOffset;
	}
	
	drawStretchedImage(tab_rect, tab_texture, BorderWidth, BorderHeight);

	if ( text )
	{
		// draw text
		font->draw(text, tab_rect, tab->getTextColor(),
			true, true, &tab_rect);
	}
		
	tab->drawImage(tab_rect);
}


//! draws the element and its children
void guiImageTabControl::draw()
{
	if ( !IsVisible )
		return;

	IGUISkin* skin = Environment->getSkin();
	
	if ( !skin )
		return;

	IGUIFont* font = skin->getFont();

	if ( !font )
		return;
	
	calcTabs();
	calcScrollButtons();
	
	guiImageTab* activeTab = 0;
		
	for (u32 side=0; side < 4; ++side)
	{
		for (s32 i=SideFirstScrollTabIndex[side]; i<=SideLastScrollTabIndex[side]; ++i)
		{
			guiImageTab* tab = SideTabs[side][i];
			
			if (tab)
			{
				if (tab->Active)
					activeTab = tab;
				else
					drawTab(tab, font);
			}
		}

		if ( SidePriorArrow[side] )
			SidePriorArrow[side]->setEnabled(SideScrollControl[side]);
		
		if ( SideNextArrow[side] )
			SideNextArrow[side]->setEnabled(SideScrollControl[side]);
	}
			
	drawStretchedImage(ContentRect, ContentTexture, BorderWidth, BorderHeight);
	
	if (activeTab)
		drawTab(activeTab, font);
		
	refreshSprites();

	IGUIElement::draw();
	
	//Environment->getVideoDriver()->draw2DRectangle(video::SColor(32,255,0,0), ContentRect, 0);
}


//! Set the height of the tabs
void guiImageTabControl::setTabHeight( s32 height )
{
	if ( height < 0 )
		height = 0;

	TabHeight = height;
}


//! Get the height of the tabs
s32 guiImageTabControl::getTabHeight() const
{
	return TabHeight;
}

//! set the maximal width of a tab. Per default width is 0 which means "no width restriction".
void guiImageTabControl::setTabWidth(s32 width )
{
	TabWidth = width;
}

//! get the maximal width of a tab
s32 guiImageTabControl::getTabWidth() const
{
	return TabWidth;
}


//! Set the extra width added to tabs on each side of the text
void guiImageTabControl::setTabPadding( s32 padding )
{
	if ( padding < 0 )
		padding = 0;

	TabPadding = padding;
}


//! Get the extra width added to tabs on each side of the text
s32 guiImageTabControl::getTabPadding() const
{
	return TabPadding;
}


//! Set the alignment of the tabs
void guiImageTabControl::setTabVerticalAlignment( EGUI_ALIGNMENT alignment )
{
	VerticalAlignment = alignment;

	core::rect<s32> r(calcRelativeRect());
	for ( u32 i=0; i<Tabs.size(); ++i )
	{
		Tabs[i]->setRelativePosition(r);
	}
}

//! Get the alignment of the tabs
EGUI_ALIGNMENT guiImageTabControl::getTabVerticalAlignment() const
{
	return VerticalAlignment;
}


s32 guiImageTabControl::getTabAt(s32 xpos, s32 ypos) const
{
	core::position2di p(xpos, ypos);

	for (u32 side=0; side < 4; ++side)
	{
		for (s32 i=SideFirstScrollTabIndex[side]; i<=SideLastScrollTabIndex[side]; ++i)
		{
			guiImageTab* tab = SideTabs[side][i];
			
			if ( tab )
			{
				if ( tab->Drawn
					 && tab->DrawnRect.isPointInside(p))
				{
					return tab->getNumber();
				}
			}
		}
	}
	
	return -1;
}

//! Returns which tab is currently active
s32 guiImageTabControl::getActiveTab() const
{
	return ActiveTabIndex;
}


//! Brings a tab to front.
bool guiImageTabControl::setActiveTab(s32 idx)
{
	if ((u32)idx >= Tabs.size())
		return false;

	bool changed = (ActiveTabIndex != idx);

	ActiveTabIndex = idx;

	for (s32 i=0; i<(s32)Tabs.size(); ++i)
		if (Tabs[i])
			Tabs[i]->setVisible( i == ActiveTabIndex );

	if (changed)
	{
		SEvent event;
		event.EventType = EET_GUI_EVENT;
		event.GUIEvent.Caller = this;
		event.GUIEvent.Element = 0;
		event.GUIEvent.EventType = EGET_TAB_CHANGED;
		Parent->OnEvent(event);
	}

	return true;
}


bool guiImageTabControl::setActiveTab(IGUITab *tab)
{
	for (s32 i=0; i<(s32)Tabs.size(); ++i)
		if (Tabs[i] == tab)
			return setActiveTab(i);
	return false;
}


//! Removes a child.
void guiImageTabControl::removeChild(IGUIElement* child)
{
	bool isTab = false;

	u32 i=0;
	// check if it is a tab
	while (i<Tabs.size())
	{
		if (Tabs[i] == child)
		{
			Tabs[i]->drop();
			Tabs.erase(i);
			for (u32 side=0; side < 4; ++side)
			{
				for (u32 j=0; j < SideTabs[side].size(); ++j)
				{
					if (SideTabs[side][j] == child)
					{
						SideTabs[side].erase(j);
						break;
					}
				}
			}
			isTab = true;
		}
		else
			++i;
	}

	// reassign numbers
	if (isTab)
	{
		for (i=0; i<Tabs.size(); ++i)
			if (Tabs[i])
				Tabs[i]->setNumber(i);
	}

	// remove real element
	IGUIElement::removeChild(child);
}


//! Update the position of the element, decides scroll button status
void guiImageTabControl::updateAbsolutePosition()
{
	IGUIElement::updateAbsolutePosition();
}


//! Writes attributes of the element.
void guiImageTabControl::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const
{
	IGUITabControl::serializeAttributes(out,options);

	out->addInt ("ActiveTabIndex",	ActiveTabIndex);
	out->addInt ("TabHeight",	TabHeight);
	out->addInt ("TabWidth", TabWidth);
	out->addEnum("TabVerticalAlignment", s32(VerticalAlignment), GUIAlignmentNames);
}


//! Reads attributes of the element
void guiImageTabControl::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0)
{
	ActiveTabIndex = -1;

	setTabHeight(in->getAttributeAsInt("TabHeight"));
	TabWidth     = in->getAttributeAsInt("TabWidth");

	IGUITabControl::deserializeAttributes(in,options);

	setActiveTab(in->getAttributeAsInt("ActiveTabIndex"));
	setTabVerticalAlignment( static_cast<EGUI_ALIGNMENT>(in->getAttributeAsEnumeration("TabVerticalAlignment" , GUIAlignmentNames)) );
}

//! Draws a stretched image
void guiImageTabControl::drawStretchedImage(const irr::core::rect<s32>& drawn_rect, 
	const video::ITexture* drawn_texture, s32 border_width, s32 border_height)
{
	if (drawn_texture)
	{
		video::IVideoDriver* driver = Environment->getVideoDriver();
	
		s32 texture_width = drawn_texture->getSize().Width;
		s32 texture_height = drawn_texture->getSize().Height;
		
		s32 left = drawn_rect.UpperLeftCorner.X;
		s32 right = drawn_rect.LowerRightCorner.X;
		s32 top = drawn_rect.UpperLeftCorner.Y;
		s32 bottom = drawn_rect.LowerRightCorner.Y;
		
		driver->draw2DImage(drawn_texture,
			irr::core::rect<s32>(left, top, left + border_width, top + border_height), 
			irr::core::rect<s32>(0, 0, border_width, border_height), 
			0, 0, true);

		driver->draw2DImage(drawn_texture,
			irr::core::rect<s32>(left + border_width, top, right - border_width, top + border_height), 
			irr::core::rect<s32>(border_width, 0, texture_width - border_width, border_height),  
			0, 0, true);

		driver->draw2DImage(drawn_texture,
			irr::core::rect<s32>(right - border_width, top, right, top + border_height), 
			irr::core::rect<s32>(texture_width - border_width, 0, texture_width, border_height),  
			0, 0, true);

		driver->draw2DImage(drawn_texture,
			irr::core::rect<s32>(left, top + border_height, left + border_width, bottom - border_height), 
			irr::core::rect<s32>(0, border_height, border_width, texture_height - border_height),  
			0, 0, true);

		driver->draw2DImage(drawn_texture,
			irr::core::rect<s32>(left + border_width, top + border_height, right - border_width, bottom - border_height), 
			irr::core::rect<s32>(border_width, border_height, texture_width - border_width, texture_height - border_height),  
			0, 0, true);

		driver->draw2DImage(drawn_texture,
			irr::core::rect<s32>(right - border_width, top + border_height, right, bottom - border_height), 
			irr::core::rect<s32>(texture_width - border_width, border_height, texture_width, texture_height - border_height),  
			0, 0, true);

		driver->draw2DImage(drawn_texture,
			irr::core::rect<s32>(left, bottom - border_height, left + border_width, bottom), 
			irr::core::rect<s32>(0, texture_height - border_height, border_width, texture_height),  
			0, 0, true);
			
		driver->draw2DImage(drawn_texture,
			irr::core::rect<s32>(left + border_width, bottom - border_height, right - border_width, bottom), 
			irr::core::rect<s32>(border_width, texture_height - border_height, texture_width - border_width, texture_height),  
			0, 0, true);
			
		driver->draw2DImage(drawn_texture,
			irr::core::rect<s32>(right - border_width, bottom - border_height, right, bottom), 
			irr::core::rect<s32>(texture_width - border_width, texture_height - border_height, texture_width, texture_height),  
			0, 0, true);
	}
}
} // end namespace irr
} // end namespace gui



