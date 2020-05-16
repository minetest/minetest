// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_GUI_ELEMENT_H_INCLUDED__
#define __I_GUI_ELEMENT_H_INCLUDED__

#include "IAttributeExchangingObject.h"
#include "irrList.h"
#include "rect.h"
#include "irrString.h"
#include "IEventReceiver.h"
#include "EGUIElementTypes.h"
#include "EGUIAlignment.h"
#include "IAttributes.h"

namespace irr
{
namespace gui
{

class IGUIEnvironment;

//! Base class of all GUI elements.
class IGUIElement : public virtual io::IAttributeExchangingObject, public IEventReceiver
{
public:

	//! Constructor
	IGUIElement(EGUI_ELEMENT_TYPE type, IGUIEnvironment* environment, IGUIElement* parent,
		s32 id, const core::rect<s32>& rectangle)
		: Parent(0), RelativeRect(rectangle), AbsoluteRect(rectangle),
		AbsoluteClippingRect(rectangle), DesiredRect(rectangle),
		MaxSize(0,0), MinSize(1,1), IsVisible(true), IsEnabled(true),
		IsSubElement(false), NoClip(false), ID(id), IsTabStop(false), TabOrder(-1), IsTabGroup(false),
		AlignLeft(EGUIA_UPPERLEFT), AlignRight(EGUIA_UPPERLEFT), AlignTop(EGUIA_UPPERLEFT), AlignBottom(EGUIA_UPPERLEFT),
		Environment(environment), Type(type)
	{
		#ifdef _DEBUG
		setDebugName("IGUIElement");
		#endif

		// if we were given a parent to attach to
		if (parent)
		{
			parent->addChildToEnd(this);
			recalculateAbsolutePosition(true);
		}
	}


	//! Destructor
	virtual ~IGUIElement()
	{
		// delete all children
		core::list<IGUIElement*>::Iterator it = Children.begin();
		for (; it != Children.end(); ++it)
		{
			(*it)->Parent = 0;
			(*it)->drop();
		}
	}


	//! Returns parent of this element.
	IGUIElement* getParent() const
	{
		return Parent;
	}


	//! Returns the relative rectangle of this element.
	core::rect<s32> getRelativePosition() const
	{
		return RelativeRect;
	}


	//! Sets the relative rectangle of this element.
	/** \param r The absolute position to set */
	void setRelativePosition(const core::rect<s32>& r)
	{
		if (Parent)
		{
			const core::rect<s32>& r2 = Parent->getAbsolutePosition();

			core::dimension2df d((f32)(r2.getSize().Width), (f32)(r2.getSize().Height));

			if (AlignLeft   == EGUIA_SCALE)
				ScaleRect.UpperLeftCorner.X = (f32)r.UpperLeftCorner.X / d.Width;
			if (AlignRight  == EGUIA_SCALE)
				ScaleRect.LowerRightCorner.X = (f32)r.LowerRightCorner.X / d.Width;
			if (AlignTop    == EGUIA_SCALE)
				ScaleRect.UpperLeftCorner.Y = (f32)r.UpperLeftCorner.Y / d.Height;
			if (AlignBottom == EGUIA_SCALE)
				ScaleRect.LowerRightCorner.Y = (f32)r.LowerRightCorner.Y / d.Height;
		}

		DesiredRect = r;
		updateAbsolutePosition();
	}

	//! Sets the relative rectangle of this element, maintaining its current width and height
	/** \param position The new relative position to set. Width and height will not be changed. */
	void setRelativePosition(const core::position2di & position)
	{
		const core::dimension2di mySize = RelativeRect.getSize();
		const core::rect<s32> rectangle(position.X, position.Y,
						position.X + mySize.Width, position.Y + mySize.Height);
		setRelativePosition(rectangle);
	}


	//! Sets the relative rectangle of this element as a proportion of its parent's area.
	/** \note This method used to be 'void setRelativePosition(const core::rect<f32>& r)'
	\param r  The rectangle to set, interpreted as a proportion of the parent's area.
	Meaningful values are in the range [0...1], unless you intend this element to spill
	outside its parent. */
	void setRelativePositionProportional(const core::rect<f32>& r)
	{
		if (!Parent)
			return;

		const core::dimension2di& d = Parent->getAbsolutePosition().getSize();

		DesiredRect = core::rect<s32>(
					core::floor32((f32)d.Width * r.UpperLeftCorner.X),
					core::floor32((f32)d.Height * r.UpperLeftCorner.Y),
					core::floor32((f32)d.Width * r.LowerRightCorner.X),
					core::floor32((f32)d.Height * r.LowerRightCorner.Y));

		ScaleRect = r;

		updateAbsolutePosition();
	}


	//! Gets the absolute rectangle of this element
	core::rect<s32> getAbsolutePosition() const
	{
		return AbsoluteRect;
	}


	//! Returns the visible area of the element.
	core::rect<s32> getAbsoluteClippingRect() const
	{
		return AbsoluteClippingRect;
	}


	//! Sets whether the element will ignore its parent's clipping rectangle
	/** \param noClip If true, the element will not be clipped by its parent's clipping rectangle. */
	void setNotClipped(bool noClip)
	{
		NoClip = noClip;
		updateAbsolutePosition();
	}


	//! Gets whether the element will ignore its parent's clipping rectangle
	/** \return true if the element is not clipped by its parent's clipping rectangle. */
	bool isNotClipped() const
	{
		return NoClip;
	}


	//! Sets the maximum size allowed for this element
	/** If set to 0,0, there is no maximum size */
	void setMaxSize(core::dimension2du size)
	{
		MaxSize = size;
		updateAbsolutePosition();
	}


	//! Sets the minimum size allowed for this element
	void setMinSize(core::dimension2du size)
	{
		MinSize = size;
		if (MinSize.Width < 1)
			MinSize.Width = 1;
		if (MinSize.Height < 1)
			MinSize.Height = 1;
		updateAbsolutePosition();
	}


	//! The alignment defines how the borders of this element will be positioned when the parent element is resized.
	void setAlignment(EGUI_ALIGNMENT left, EGUI_ALIGNMENT right, EGUI_ALIGNMENT top, EGUI_ALIGNMENT bottom)
	{
		AlignLeft = left;
		AlignRight = right;
		AlignTop = top;
		AlignBottom = bottom;

		if (Parent)
		{
			core::rect<s32> r(Parent->getAbsolutePosition());

			core::dimension2df d((f32)r.getSize().Width, (f32)r.getSize().Height);

			if (AlignLeft   == EGUIA_SCALE)
				ScaleRect.UpperLeftCorner.X = (f32)DesiredRect.UpperLeftCorner.X / d.Width;
			if (AlignRight  == EGUIA_SCALE)
				ScaleRect.LowerRightCorner.X = (f32)DesiredRect.LowerRightCorner.X / d.Width;
			if (AlignTop    == EGUIA_SCALE)
				ScaleRect.UpperLeftCorner.Y = (f32)DesiredRect.UpperLeftCorner.Y / d.Height;
			if (AlignBottom == EGUIA_SCALE)
				ScaleRect.LowerRightCorner.Y = (f32)DesiredRect.LowerRightCorner.Y / d.Height;
		}
	}


	//! Updates the absolute position.
	virtual void updateAbsolutePosition()
	{
		recalculateAbsolutePosition(false);

		// update all children
		core::list<IGUIElement*>::Iterator it = Children.begin();
		for (; it != Children.end(); ++it)
		{
			(*it)->updateAbsolutePosition();
		}
	}


	//! Returns the topmost GUI element at the specific position.
	/**
	This will check this GUI element and all of its descendants, so it
	may return this GUI element.  To check all GUI elements, call this
	function on device->getGUIEnvironment()->getRootGUIElement(). Note
	that the root element is the size of the screen, so doing so (with
	an on-screen point) will always return the root element if no other
	element is above it at that point.
	\param point: The point at which to find a GUI element.
	\return The topmost GUI element at that point, or 0 if there are
	no candidate elements at this point.
	*/
	IGUIElement* getElementFromPoint(const core::position2d<s32>& point)
	{
		IGUIElement* target = 0;

		// we have to search from back to front, because later children
		// might be drawn over the top of earlier ones.

		core::list<IGUIElement*>::Iterator it = Children.getLast();

		if (isVisible())
		{
			while(it != Children.end())
			{
				target = (*it)->getElementFromPoint(point);
				if (target)
					return target;

				--it;
			}
		}

		if (isVisible() && isPointInside(point))
			target = this;

		return target;
	}


	//! Returns true if a point is within this element.
	/** Elements with a shape other than a rectangle should override this method */
	virtual bool isPointInside(const core::position2d<s32>& point) const
	{
		return AbsoluteClippingRect.isPointInside(point);
	}


	//! Adds a GUI element as new child of this element.
	virtual void addChild(IGUIElement* child)
	{
		addChildToEnd(child);
		if (child)
		{
			child->updateAbsolutePosition();
		}
	}

	//! Removes a child.
	virtual void removeChild(IGUIElement* child)
	{
		core::list<IGUIElement*>::Iterator it = Children.begin();
		for (; it != Children.end(); ++it)
			if ((*it) == child)
			{
				(*it)->Parent = 0;
				(*it)->drop();
				Children.erase(it);
				return;
			}
	}


	//! Removes this element from its parent.
	virtual void remove()
	{
		if (Parent)
			Parent->removeChild(this);
	}


	//! Draws the element and its children.
	virtual void draw()
	{
		if ( isVisible() )
		{
			core::list<IGUIElement*>::Iterator it = Children.begin();
			for (; it != Children.end(); ++it)
				(*it)->draw();
		}
	}


	//! animate the element and its children.
	virtual void OnPostRender(u32 timeMs)
	{
		if ( isVisible() )
		{
			core::list<IGUIElement*>::Iterator it = Children.begin();
			for (; it != Children.end(); ++it)
				(*it)->OnPostRender( timeMs );
		}
	}


	//! Moves this element.
	virtual void move(core::position2d<s32> absoluteMovement)
	{
		setRelativePosition(DesiredRect + absoluteMovement);
	}


	//! Returns true if element is visible.
	virtual bool isVisible() const
	{
		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return IsVisible;
	}


	//! Sets the visible state of this element.
	virtual void setVisible(bool visible)
	{
		IsVisible = visible;
	}


	//! Returns true if this element was created as part of its parent control
	virtual bool isSubElement() const
	{
		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return IsSubElement;
	}


	//! Sets whether this control was created as part of its parent.
	/** For example, it is true when a scrollbar is part of a listbox.
	SubElements are not saved to disk when calling guiEnvironment->saveGUI() */
	virtual void setSubElement(bool subElement)
	{
		IsSubElement = subElement;
	}


	//! If set to true, the focus will visit this element when using the tab key to cycle through elements.
	/** If this element is a tab group (see isTabGroup/setTabGroup) then
	ctrl+tab will be used instead. */
	void setTabStop(bool enable)
	{
		IsTabStop = enable;
	}


	//! Returns true if this element can be focused by navigating with the tab key
	bool isTabStop() const
	{
		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return IsTabStop;
	}


	//! Sets the priority of focus when using the tab key to navigate between a group of elements.
	/** See setTabGroup, isTabGroup and getTabGroup for information on tab groups.
	Elements with a lower number are focused first */
	void setTabOrder(s32 index)
	{
		// negative = autonumber
		if (index < 0)
		{
			TabOrder = 0;
			IGUIElement *el = getTabGroup();
			while (IsTabGroup && el && el->Parent)
				el = el->Parent;

			IGUIElement *first=0, *closest=0;
			if (el)
			{
				// find the highest element number
				el->getNextElement(-1, true, IsTabGroup, first, closest, true);
				if (first)
				{
					TabOrder = first->getTabOrder() + 1;
				}
			}

		}
		else
			TabOrder = index;
	}


	//! Returns the number in the tab order sequence
	s32 getTabOrder() const
	{
		return TabOrder;
	}


	//! Sets whether this element is a container for a group of elements which can be navigated using the tab key.
	/** For example, windows are tab groups.
	Groups can be navigated using ctrl+tab, providing isTabStop is true. */
	void setTabGroup(bool isGroup)
	{
		IsTabGroup = isGroup;
	}


	//! Returns true if this element is a tab group.
	bool isTabGroup() const
	{
		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return IsTabGroup;
	}


	//! Returns the container element which holds all elements in this element's tab group.
	IGUIElement* getTabGroup()
	{
		IGUIElement *ret=this;

		while (ret && !ret->isTabGroup())
			ret = ret->getParent();

		return ret;
	}


	//! Returns true if element is enabled
	/** Currently elements do _not_ care about parent-states.
		So if you want to affect childs you have to enable/disable them all.
		The only exception to this are sub-elements which also check their parent.
	*/
	virtual bool isEnabled() const
	{
		if ( isSubElement() && IsEnabled && getParent() )
			return getParent()->isEnabled();

		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return IsEnabled;
	}


	//! Sets the enabled state of this element.
	virtual void setEnabled(bool enabled)
	{
		IsEnabled = enabled;
	}


	//! Sets the new caption of this element.
	virtual void setText(const wchar_t* text)
	{
		Text = text;
	}


	//! Returns caption of this element.
	virtual const wchar_t* getText() const
	{
		return Text.c_str();
	}


	//! Sets the new caption of this element.
	virtual void setToolTipText(const wchar_t* text)
	{
		ToolTipText = text;
	}


	//! Returns caption of this element.
	virtual const core::stringw& getToolTipText() const
	{
		return ToolTipText;
	}


	//! Returns id. Can be used to identify the element.
	virtual s32 getID() const
	{
		return ID;
	}


	//! Sets the id of this element
	virtual void setID(s32 id)
	{
		ID = id;
	}


	//! Called if an event happened.
	virtual bool OnEvent(const SEvent& event)
	{
		return Parent ? Parent->OnEvent(event) : false;
	}


	//! Brings a child to front
	/** \return True if successful, false if not. */
	virtual bool bringToFront(IGUIElement* element)
	{
		core::list<IGUIElement*>::Iterator it = Children.begin();
		for (; it != Children.end(); ++it)
		{
			if (element == (*it))
			{
				Children.erase(it);
				Children.push_back(element);
				return true;
			}
		}

		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return false;
	}


	//! Moves a child to the back, so it's siblings are drawn on top of it
	/** \return True if successful, false if not. */
	virtual bool sendToBack(IGUIElement* child)
	{
		core::list<IGUIElement*>::Iterator it = Children.begin();
		if (child == (*it))	// already there
			return true;
		for (; it != Children.end(); ++it)
		{
			if (child == (*it))
			{
				Children.erase(it);
				Children.push_front(child);
				return true;
			}
		}

		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return false;
	}

	//! Returns list with children of this element
	virtual const core::list<IGUIElement*>& getChildren() const
	{
		return Children;
	}


	//! Finds the first element with the given id.
	/** \param id: Id to search for.
	\param searchchildren: Set this to true, if also children of this
	element may contain the element with the searched id and they
	should be searched too.
	\return Returns the first element with the given id. If no element
	with this id was found, 0 is returned. */
	virtual IGUIElement* getElementFromId(s32 id, bool searchchildren=false) const
	{
		IGUIElement* e = 0;

		core::list<IGUIElement*>::ConstIterator it = Children.begin();
		for (; it != Children.end(); ++it)
		{
			if ((*it)->getID() == id)
				return (*it);

			if (searchchildren)
				e = (*it)->getElementFromId(id, true);

			if (e)
				return e;
		}

		return e;
	}


	//! returns true if the given element is a child of this one.
	//! \param child: The child element to check
	bool isMyChild(IGUIElement* child) const
	{
		if (!child)
			return false;
		do
		{
			if (child->Parent)
				child = child->Parent;

		} while (child->Parent && child != this);

		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return child == this;
	}


	//! searches elements to find the closest next element to tab to
	/** \param startOrder: The TabOrder of the current element, -1 if none
	\param reverse: true if searching for a lower number
	\param group: true if searching for a higher one
	\param first: element with the highest/lowest known tab order depending on search direction
	\param closest: the closest match, depending on tab order and direction
	\param includeInvisible: includes invisible elements in the search (default=false)
	\return true if successfully found an element, false to continue searching/fail */
	bool getNextElement(s32 startOrder, bool reverse, bool group,
		IGUIElement*& first, IGUIElement*& closest, bool includeInvisible=false) const
	{
		// we'll stop searching if we find this number
		s32 wanted = startOrder + ( reverse ? -1 : 1 );
		if (wanted==-2)
			wanted = 1073741824; // maximum s32

		core::list<IGUIElement*>::ConstIterator it = Children.begin();

		s32 closestOrder, currentOrder;

		while(it != Children.end())
		{
			// ignore invisible elements and their children
			if ( ( (*it)->isVisible() || includeInvisible ) &&
				(group == true || (*it)->isTabGroup() == false) )
			{
				// only check tab stops and those with the same group status
				if ((*it)->isTabStop() && ((*it)->isTabGroup() == group))
				{
					currentOrder = (*it)->getTabOrder();

					// is this what we're looking for?
					if (currentOrder == wanted)
					{
						closest = *it;
						return true;
					}

					// is it closer than the current closest?
					if (closest)
					{
						closestOrder = closest->getTabOrder();
						if ( ( reverse && currentOrder > closestOrder && currentOrder < startOrder)
							||(!reverse && currentOrder < closestOrder && currentOrder > startOrder))
						{
							closest = *it;
						}
					}
					else
					if ( (reverse && currentOrder < startOrder) || (!reverse && currentOrder > startOrder) )
					{
						closest = *it;
					}

					// is it before the current first?
					if (first)
					{
						closestOrder = first->getTabOrder();

						if ( (reverse && closestOrder < currentOrder) || (!reverse && closestOrder > currentOrder) )
						{
							first = *it;
						}
					}
					else
					{
						first = *it;
					}
				}
				// search within children
				if ((*it)->getNextElement(startOrder, reverse, group, first, closest))
				{
					_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
					return true;
				}
			}
			++it;
		}
		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return false;
	}


	//! Returns the type of the gui element.
	/** This is needed for the .NET wrapper but will be used
	later for serializing and deserializing.
	If you wrote your own GUIElements, you need to set the type for your element as first parameter
	in the constructor of IGUIElement. For own (=unknown) elements, simply use EGUIET_ELEMENT as type */
	EGUI_ELEMENT_TYPE getType() const
	{
		return Type;
	}

	//! Returns true if the gui element supports the given type.
	/** This is mostly used to check if you can cast a gui element to the class that goes with the type.
	Most gui elements will only support their own type, but if you derive your own classes from interfaces
	you can overload this function and add a check for the type of the base-class additionally.
	This allows for checks comparable to the dynamic_cast of c++ with enabled rtti.
	Note that you can't do that by calling BaseClass::hasType(type), but you have to do an explicit
	comparison check, because otherwise the base class usually just checks for the membervariable
	Type which contains the type of your derived class.
	*/
	virtual bool hasType(EGUI_ELEMENT_TYPE type) const
	{
		return type == Type;
	}


	//! Returns the type name of the gui element.
	/** This is needed serializing elements. For serializing your own elements, override this function
	and return your own type name which is created by your IGUIElementFactory */
	virtual const c8* getTypeName() const
	{
		return GUIElementTypeNames[Type];
	}
	
	//! Returns the name of the element.
	/** \return Name as character string. */
	virtual const c8* getName() const
	{
		return Name.c_str();
	}


	//! Sets the name of the element.
	/** \param name New name of the gui element. */
	virtual void setName(const c8* name)
	{
		Name = name;
	}


	//! Sets the name of the element.
	/** \param name New name of the gui element. */
	virtual void setName(const core::stringc& name)
	{
		Name = name;
	}


	//! Writes attributes of the scene node.
	/** Implement this to expose the attributes of your scene node for
	scripting languages, editors, debuggers or xml serialization purposes. */
	virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const
	{
		out->addString("Name", Name.c_str());		
		out->addInt("Id", ID );
		out->addString("Caption", getText());
		out->addRect("Rect", DesiredRect);
		out->addPosition2d("MinSize", core::position2di(MinSize.Width, MinSize.Height));
		out->addPosition2d("MaxSize", core::position2di(MaxSize.Width, MaxSize.Height));
		out->addEnum("LeftAlign", AlignLeft, GUIAlignmentNames);
		out->addEnum("RightAlign", AlignRight, GUIAlignmentNames);
		out->addEnum("TopAlign", AlignTop, GUIAlignmentNames);
		out->addEnum("BottomAlign", AlignBottom, GUIAlignmentNames);
		out->addBool("Visible", IsVisible);
		out->addBool("Enabled", IsEnabled);
		out->addBool("TabStop", IsTabStop);
		out->addBool("TabGroup", IsTabGroup);
		out->addInt("TabOrder", TabOrder);
		out->addBool("NoClip", NoClip);
	}


	//! Reads attributes of the scene node.
	/** Implement this to set the attributes of your scene node for
	scripting languages, editors, debuggers or xml deserialization purposes. */
	virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0)
	{
		setName(in->getAttributeAsString("Name"));
		setID(in->getAttributeAsInt("Id"));
		setText(in->getAttributeAsStringW("Caption").c_str());
		setVisible(in->getAttributeAsBool("Visible"));
		setEnabled(in->getAttributeAsBool("Enabled"));
		IsTabStop = in->getAttributeAsBool("TabStop");
		IsTabGroup = in->getAttributeAsBool("TabGroup");
		TabOrder = in->getAttributeAsInt("TabOrder");

		core::position2di p = in->getAttributeAsPosition2d("MaxSize");
		setMaxSize(core::dimension2du(p.X,p.Y));

		p = in->getAttributeAsPosition2d("MinSize");
		setMinSize(core::dimension2du(p.X,p.Y));

		setAlignment((EGUI_ALIGNMENT) in->getAttributeAsEnumeration("LeftAlign", GUIAlignmentNames),
			(EGUI_ALIGNMENT)in->getAttributeAsEnumeration("RightAlign", GUIAlignmentNames),
			(EGUI_ALIGNMENT)in->getAttributeAsEnumeration("TopAlign", GUIAlignmentNames),
			(EGUI_ALIGNMENT)in->getAttributeAsEnumeration("BottomAlign", GUIAlignmentNames));

		setRelativePosition(in->getAttributeAsRect("Rect"));

		setNotClipped(in->getAttributeAsBool("NoClip"));
	}

protected:
	// not virtual because needed in constructor
	void addChildToEnd(IGUIElement* child)
	{
		if (child)
		{
			child->grab(); // prevent destruction when removed
			child->remove(); // remove from old parent
			child->LastParentRect = getAbsolutePosition();
			child->Parent = this;
			Children.push_back(child);
		}
	}

	// not virtual because needed in constructor
	void recalculateAbsolutePosition(bool recursive)
	{
		core::rect<s32> parentAbsolute(0,0,0,0);
		core::rect<s32> parentAbsoluteClip;
		f32 fw=0.f, fh=0.f;

		if (Parent)
		{
			parentAbsolute = Parent->AbsoluteRect;

			if (NoClip)
			{
				IGUIElement* p=this;
				while (p && p->Parent)
					p = p->Parent;
				parentAbsoluteClip = p->AbsoluteClippingRect;
			}
			else
				parentAbsoluteClip = Parent->AbsoluteClippingRect;
		}

		const s32 diffx = parentAbsolute.getWidth() - LastParentRect.getWidth();
		const s32 diffy = parentAbsolute.getHeight() - LastParentRect.getHeight();

		if (AlignLeft == EGUIA_SCALE || AlignRight == EGUIA_SCALE)
			fw = (f32)parentAbsolute.getWidth();

		if (AlignTop == EGUIA_SCALE || AlignBottom == EGUIA_SCALE)
			fh = (f32)parentAbsolute.getHeight();

		switch (AlignLeft)
		{
			case EGUIA_UPPERLEFT:
				break;
			case EGUIA_LOWERRIGHT:
				DesiredRect.UpperLeftCorner.X += diffx;
				break;
			case EGUIA_CENTER:
				DesiredRect.UpperLeftCorner.X += diffx/2;
				break;
			case EGUIA_SCALE:
				DesiredRect.UpperLeftCorner.X = core::round32(ScaleRect.UpperLeftCorner.X * fw);
				break;
		}

		switch (AlignRight)
		{
			case EGUIA_UPPERLEFT:
				break;
			case EGUIA_LOWERRIGHT:
				DesiredRect.LowerRightCorner.X += diffx;
				break;
			case EGUIA_CENTER:
				DesiredRect.LowerRightCorner.X += diffx/2;
				break;
			case EGUIA_SCALE:
				DesiredRect.LowerRightCorner.X = core::round32(ScaleRect.LowerRightCorner.X * fw);
				break;
		}

		switch (AlignTop)
		{
			case EGUIA_UPPERLEFT:
				break;
			case EGUIA_LOWERRIGHT:
				DesiredRect.UpperLeftCorner.Y += diffy;
				break;
			case EGUIA_CENTER:
				DesiredRect.UpperLeftCorner.Y += diffy/2;
				break;
			case EGUIA_SCALE:
				DesiredRect.UpperLeftCorner.Y = core::round32(ScaleRect.UpperLeftCorner.Y * fh);
				break;
		}

		switch (AlignBottom)
		{
			case EGUIA_UPPERLEFT:
				break;
			case EGUIA_LOWERRIGHT:
				DesiredRect.LowerRightCorner.Y += diffy;
				break;
			case EGUIA_CENTER:
				DesiredRect.LowerRightCorner.Y += diffy/2;
				break;
			case EGUIA_SCALE:
				DesiredRect.LowerRightCorner.Y = core::round32(ScaleRect.LowerRightCorner.Y * fh);
				break;
		}

		RelativeRect = DesiredRect;

		const s32 w = RelativeRect.getWidth();
		const s32 h = RelativeRect.getHeight();

		// make sure the desired rectangle is allowed
		if (w < (s32)MinSize.Width)
			RelativeRect.LowerRightCorner.X = RelativeRect.UpperLeftCorner.X + MinSize.Width;
		if (h < (s32)MinSize.Height)
			RelativeRect.LowerRightCorner.Y = RelativeRect.UpperLeftCorner.Y + MinSize.Height;
		if (MaxSize.Width && w > (s32)MaxSize.Width)
			RelativeRect.LowerRightCorner.X = RelativeRect.UpperLeftCorner.X + MaxSize.Width;
		if (MaxSize.Height && h > (s32)MaxSize.Height)
			RelativeRect.LowerRightCorner.Y = RelativeRect.UpperLeftCorner.Y + MaxSize.Height;

		RelativeRect.repair();

		AbsoluteRect = RelativeRect + parentAbsolute.UpperLeftCorner;

		if (!Parent)
			parentAbsoluteClip = AbsoluteRect;

		AbsoluteClippingRect = AbsoluteRect;
		AbsoluteClippingRect.clipAgainst(parentAbsoluteClip);

		LastParentRect = parentAbsolute;

		if ( recursive )
		{
			// update all children
			core::list<IGUIElement*>::Iterator it = Children.begin();
			for (; it != Children.end(); ++it)
			{
				(*it)->recalculateAbsolutePosition(recursive);
			}
		}
	}

protected:

	//! List of all children of this element
	core::list<IGUIElement*> Children;

	//! Pointer to the parent
	IGUIElement* Parent;

	//! relative rect of element
	core::rect<s32> RelativeRect;

	//! absolute rect of element
	core::rect<s32> AbsoluteRect;

	//! absolute clipping rect of element
	core::rect<s32> AbsoluteClippingRect;

	//! the rectangle the element would prefer to be,
	//! if it was not constrained by parent or max/min size
	core::rect<s32> DesiredRect;

	//! for calculating the difference when resizing parent
	core::rect<s32> LastParentRect;

	//! relative scale of the element inside its parent
	core::rect<f32> ScaleRect;

	//! maximum and minimum size of the element
	core::dimension2du MaxSize, MinSize;

	//! is visible?
	bool IsVisible;

	//! is enabled?
	bool IsEnabled;

	//! is a part of a larger whole and should not be serialized?
	bool IsSubElement;

	//! does this element ignore its parent's clipping rectangle?
	bool NoClip;

	//! caption
	core::stringw Text;

	//! tooltip
	core::stringw ToolTipText;
	
	//! users can set this for identificating the element by string
	core::stringc Name;

	//! users can set this for identificating the element by integer
	s32 ID;

	//! tab stop like in windows
	bool IsTabStop;

	//! tab order
	s32 TabOrder;

	//! tab groups are containers like windows, use ctrl+tab to navigate
	bool IsTabGroup;

	//! tells the element how to act when its parent is resized
	EGUI_ALIGNMENT AlignLeft, AlignRight, AlignTop, AlignBottom;

	//! GUI Environment
	IGUIEnvironment* Environment;

	//! type of element
	EGUI_ELEMENT_TYPE Type;
};


} // end namespace gui
} // end namespace irr

#endif

