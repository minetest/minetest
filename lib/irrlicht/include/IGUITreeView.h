// written by Reinhard Ostermeier, reinhard@nospam.r-ostermeier.de
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_GUI_TREE_VIEW_H_INCLUDED__
#define __I_GUI_TREE_VIEW_H_INCLUDED__

#include "IGUIElement.h"
#include "IGUIImageList.h"
#include "irrTypes.h"

namespace irr
{
namespace gui
{
	class IGUIFont;
	class IGUITreeView;


	//! Node for gui tree view
	/** \par This element can create the following events of type EGUI_EVENT_TYPE:
	\li EGET_TREEVIEW_NODE_EXPAND
	\li EGET_TREEVIEW_NODE_COLLAPS
	\li EGET_TREEVIEW_NODE_DESELECT
	\li EGET_TREEVIEW_NODE_SELECT
	*/
	class IGUITreeViewNode : public IReferenceCounted
	{
	public:
		//! returns the owner (tree view) of this node
		virtual IGUITreeView* getOwner() const = 0;

		//! Returns the parent node of this node.
		/** For the root node this will return 0. */
		virtual IGUITreeViewNode* getParent() const = 0;

		//! returns the text of the node
		virtual const wchar_t* getText() const = 0;

		//! sets the text of the node
		virtual void setText( const wchar_t* text ) = 0;

		//! returns the icon text of the node
		virtual const wchar_t* getIcon() const = 0;

		//! sets the icon text of the node
		virtual void setIcon( const wchar_t* icon ) = 0;

		//! returns the image index of the node
		virtual u32 getImageIndex() const = 0;

		//! sets the image index of the node
		virtual void setImageIndex( u32 imageIndex ) = 0;

		//! returns the image index of the node
		virtual u32 getSelectedImageIndex() const = 0;

		//! sets the image index of the node
		virtual void setSelectedImageIndex( u32 imageIndex ) = 0;

		//! returns the user data (void*) of this node
		virtual void* getData() const = 0;

		//! sets the user data (void*) of this node
		virtual void setData( void* data ) = 0;

		//! returns the user data2 (IReferenceCounted) of this node
		virtual IReferenceCounted* getData2() const = 0;

		//! sets the user data2 (IReferenceCounted) of this node
		virtual void setData2( IReferenceCounted* data ) = 0;

		//! returns the child item count
		virtual u32 getChildCount() const = 0;

		//! removes all children (recursive) from this node
		virtual void clearChildren() = 0;

		//! removes all children (recursive) from this node
		/** \deprecated Deprecated in 1.8, use clearChildren() instead.
		This method may be removed by Irrlicht 1.9 */
		_IRR_DEPRECATED_ void clearChilds()
		{
			return clearChildren();
		}

		//! returns true if this node has child nodes
		virtual bool hasChildren() const = 0;

		//! returns true if this node has child nodes
		/** \deprecated Deprecated in 1.8, use hasChildren() instead. 
		This method may be removed by Irrlicht 1.9 */
		_IRR_DEPRECATED_ bool hasChilds() const
		{
			return hasChildren();
		}

		//! Adds a new node behind the last child node.
		/** \param text text of the new node
		\param icon icon text of the new node
		\param imageIndex index of the image for the new node (-1 = none)
		\param selectedImageIndex index of the selected image for the new node (-1 = same as imageIndex)
		\param data user data (void*) of the new node
		\param data2 user data2 (IReferenceCounted*) of the new node
		\return The new node
		*/
		virtual IGUITreeViewNode* addChildBack(
				const wchar_t* text, const wchar_t* icon = 0,
				s32 imageIndex=-1, s32 selectedImageIndex=-1,
				void* data=0, IReferenceCounted* data2=0) =0;

		//! Adds a new node before the first child node.
		/** \param text text of the new node
		\param icon icon text of the new node
		\param imageIndex index of the image for the new node (-1 = none)
		\param selectedImageIndex index of the selected image for the new node (-1 = same as imageIndex)
		\param data user data (void*) of the new node
		\param data2 user data2 (IReferenceCounted*) of the new node
		\return The new node
		*/
		virtual IGUITreeViewNode* addChildFront(
				const wchar_t* text, const wchar_t* icon = 0,
				s32 imageIndex=-1, s32 selectedImageIndex=-1,
				void* data=0, IReferenceCounted* data2=0 ) =0;

		//! Adds a new node behind the other node.
		/** The other node has also te be a child node from this node.
		\param other Node to insert after
		\param text text of the new node
		\param icon icon text of the new node
		\param imageIndex index of the image for the new node (-1 = none)
		\param selectedImageIndex index of the selected image for the new node (-1 = same as imageIndex)
		\param data user data (void*) of the new node
		\param data2 user data2 (IReferenceCounted*) of the new node
		\return The new node or 0 if other is no child node from this
		*/
		virtual IGUITreeViewNode* insertChildAfter(
				IGUITreeViewNode* other,
				const wchar_t* text, const wchar_t* icon = 0,
				s32 imageIndex=-1, s32 selectedImageIndex=-1,
				void* data=0, IReferenceCounted* data2=0) =0;

		//! Adds a new node before the other node.
		/** The other node has also te be a child node from this node.
		\param other Node to insert before
		\param text text of the new node
		\param icon icon text of the new node
		\param imageIndex index of the image for the new node (-1 = none)
		\param selectedImageIndex index of the selected image for the new node (-1 = same as imageIndex)
		\param data user data (void*) of the new node
		\param data2 user data2 (IReferenceCounted*) of the new node
		\return The new node or 0 if other is no child node from this
		*/
		virtual IGUITreeViewNode* insertChildBefore(
				IGUITreeViewNode* other,
				const wchar_t* text, const wchar_t* icon = 0,
				s32 imageIndex=-1, s32 selectedImageIndex=-1,
				void* data=0, IReferenceCounted* data2=0) = 0;

		//! Return the first child node from this node.
		/** \return The first child node or 0 if this node has no children. */
		virtual IGUITreeViewNode* getFirstChild() const = 0;

		//! Return the last child node from this node.
		/** \return The last child node or 0 if this node has no children. */
		virtual IGUITreeViewNode* getLastChild() const = 0;

		//! Returns the previous sibling node from this node.
		/** \return The previous sibling node from this node or 0 if this is
		the first node from the parent node.
		*/
		virtual IGUITreeViewNode* getPrevSibling() const = 0;

		//! Returns the next sibling node from this node.
		/** \return The next sibling node from this node or 0 if this is
		the last node from the parent node.
		*/
		virtual IGUITreeViewNode* getNextSibling() const = 0;

		//! Returns the next visible (expanded, may be out of scrolling) node from this node.
		/** \return The next visible node from this node or 0 if this is
		the last visible node. */
		virtual IGUITreeViewNode* getNextVisible() const = 0;

		//! Deletes a child node.
		/** \return Returns true if the node was found as a child and is deleted. */
		virtual bool deleteChild( IGUITreeViewNode* child ) = 0;

		//! Moves a child node one position up.
		/** \return True if the node was found as achild node and was not already the first child. */
		virtual bool moveChildUp( IGUITreeViewNode* child ) = 0;

		//! Moves a child node one position down.
		/** \return True if the node was found as achild node and was not already the last child. */
		virtual bool moveChildDown( IGUITreeViewNode* child ) = 0;

		//! Returns true if the node is expanded (children are visible).
		virtual bool getExpanded() const = 0;

		//! Sets if the node is expanded.
		virtual void setExpanded( bool expanded ) = 0;

		//! Returns true if the node is currently selected.
		virtual bool getSelected() const = 0;

		//! Sets this node as selected.
		virtual void setSelected( bool selected ) = 0;

		//! Returns true if this node is the root node.
		virtual bool isRoot() const = 0;

		//! Returns the level of this node.
		/** The root node has level 0. Direct children of the root has level 1 ... */
		virtual s32 getLevel() const = 0;

		//! Returns true if this node is visible (all parents are expanded).
		virtual bool isVisible() const = 0;
	};


	//! Default tree view GUI element.
	/** Displays a windows like tree buttons to expand/collaps the child
	nodes of an node and optional tree lines. Each node consits of an
	text, an icon text and a void pointer for user data. */
	class IGUITreeView : public IGUIElement
	{
	public:
		//! constructor
		IGUITreeView(IGUIEnvironment* environment, IGUIElement* parent,
				s32 id, core::rect<s32> rectangle)
			: IGUIElement( EGUIET_TREE_VIEW, environment, parent, id, rectangle ) {}

		//! returns the root node (not visible) from the tree.
		virtual IGUITreeViewNode* getRoot() const = 0;

		//! returns the selected node of the tree or 0 if none is selected
		virtual IGUITreeViewNode* getSelected() const = 0;

		//! returns true if the tree lines are visible
		virtual bool getLinesVisible() const = 0;

		//! sets if the tree lines are visible
		/** \param visible true for visible, false for invisible */
		virtual void setLinesVisible( bool visible ) = 0;

		//! Sets the font which should be used as icon font.
		/** This font is set to the Irrlicht engine built-in-font by
		default. Icons can be displayed in front of every list item.
		An icon is a string, displayed with the icon font. When using
		the build-in-font of the Irrlicht engine as icon font, the icon
		strings defined in GUIIcons.h can be used.
		*/
		virtual void setIconFont( IGUIFont* font ) = 0;

		//! Sets the image list which should be used for the image and selected image of every node.
		/** The default is 0 (no images). */
		virtual void setImageList( IGUIImageList* imageList ) = 0;

		//! Returns the image list which is used for the nodes.
		virtual IGUIImageList* getImageList() const = 0;

		//! Sets if the image is left of the icon. Default is true.
		virtual void setImageLeftOfIcon( bool bLeftOf ) = 0;

		//! Returns if the Image is left of the icon. Default is true.
		virtual bool getImageLeftOfIcon() const = 0;

		//! Returns the node which is associated to the last event.
		/** This pointer is only valid inside the OnEvent call! */
		virtual IGUITreeViewNode* getLastEventNode() const = 0;
	};


} // end namespace gui
} // end namespace irr

#endif

