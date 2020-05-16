// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_GUI_ELEMENT_FACTORY_H_INCLUDED__
#define __I_GUI_ELEMENT_FACTORY_H_INCLUDED__

#include "IReferenceCounted.h"
#include "EGUIElementTypes.h"

namespace irr
{

namespace gui
{
	class IGUIElement;

	//! Interface making it possible to dynamically create GUI elements
	/** To be able to add custom elements to Irrlicht and to make it possible for the
	scene manager to save and load them, simply implement this interface and register it
	in your gui environment via IGUIEnvironment::registerGUIElementFactory.
	Note: When implementing your own element factory, don't call IGUIEnvironment::grab() to
	increase the reference counter of the environment. This is not necessary because the
	it will grab() the factory anyway, and otherwise cyclic references will be created.
	*/
	class IGUIElementFactory : public virtual IReferenceCounted
	{
	public:

		//! adds an element to the gui environment based on its type id
		/** \param type: Type of the element to add.
		\param parent: Parent scene node of the new element, can be null to add to the root.
		\return Pointer to the new element or null if not successful. */
		virtual IGUIElement* addGUIElement(EGUI_ELEMENT_TYPE type, IGUIElement* parent=0) = 0;

		//! adds a GUI element to the GUI Environment based on its type name
		/** \param typeName: Type name of the element to add.
		\param parent: Parent scene node of the new element, can be null to add it to the root.
		\return Pointer to the new element or null if not successful. */
		virtual IGUIElement* addGUIElement(const c8* typeName, IGUIElement* parent=0) = 0;

		//! Get amount of GUI element types this factory is able to create
		virtual s32 getCreatableGUIElementTypeCount() const = 0;

		//! Get type of a createable element type
		/** \param idx: Index of the element type in this factory. Must be a value between 0 and
		getCreatableGUIElementTypeCount() */
		virtual EGUI_ELEMENT_TYPE getCreateableGUIElementType(s32 idx) const = 0;

		//! Get type name of a createable GUI element type by index
		/** \param idx: Index of the type in this factory. Must be a value between 0 and
		getCreatableGUIElementTypeCount() */
		virtual const c8* getCreateableGUIElementTypeName(s32 idx) const = 0;

		//! returns type name of a createable GUI element
		/** \param type: Type of GUI element.
		\return Name of the type if this factory can create the type, otherwise 0. */
		virtual const c8* getCreateableGUIElementTypeName(EGUI_ELEMENT_TYPE type) const = 0;
	};


} // end namespace gui
} // end namespace irr

#endif // __I_GUI_ELEMENT_FACTORY_H_INCLUDED__

