// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_GUI_ENVIRONMENT_H_INCLUDED__
#define __I_GUI_ENVIRONMENT_H_INCLUDED__

#include "IReferenceCounted.h"
#include "IGUISkin.h"
#include "rect.h"
#include "EMessageBoxFlags.h"
#include "IEventReceiver.h"
#include "IXMLReader.h"
#include "path.h"

namespace irr
{
	class IOSOperator;
	class IEventReceiver;

	namespace io
	{
		class IXMLWriter;
		class IReadFile;
		class IWriteFile;
		class IFileSystem;
	} // end namespace io
	namespace video
	{
		class IVideoDriver;
		class ITexture;
	} // end namespace video

namespace gui
{

class IGUIElement;
class IGUIFont;
class IGUISpriteBank;
class IGUIScrollBar;
class IGUIImage;
class IGUIMeshViewer;
class IGUICheckBox;
class IGUIListBox;
class IGUITreeView;
class IGUIImageList;
class IGUIFileOpenDialog;
class IGUIColorSelectDialog;
class IGUIInOutFader;
class IGUIStaticText;
class IGUIEditBox;
class IGUISpinBox;
class IGUITabControl;
class IGUITab;
class IGUITable;
class IGUIContextMenu;
class IGUIComboBox;
class IGUIToolBar;
class IGUIButton;
class IGUIWindow;
class IGUIElementFactory;

//! GUI Environment. Used as factory and manager of all other GUI elements.
/** \par This element can create the following events of type EGUI_EVENT_TYPE (which are passed on to focused sub-elements):
\li EGET_ELEMENT_FOCUS_LOST
\li EGET_ELEMENT_FOCUSED
\li EGET_ELEMENT_LEFT
\li EGET_ELEMENT_HOVERED
*/
class IGUIEnvironment : public virtual IReferenceCounted
{
public:

	//! Draws all gui elements by traversing the GUI environment starting at the root node.
	virtual void drawAll() = 0;

	//! Sets the focus to an element.
	/** Causes a EGET_ELEMENT_FOCUS_LOST event followed by a
	EGET_ELEMENT_FOCUSED event. If someone absorbed either of the events,
	then the focus will not be changed.
	\param element Pointer to the element which shall get the focus.
	\return True on success, false on failure */
	virtual bool setFocus(IGUIElement* element) = 0;

	//! Returns the element which holds the focus.
	/** \return Pointer to the element with focus. */
	virtual IGUIElement* getFocus() const = 0;

	//! Returns the element which was last under the mouse cursor
	/** NOTE: This information is updated _after_ the user-eventreceiver 
	received it's mouse-events. To find the hovered element while catching 
	mouse events you have to use instead:
	IGUIEnvironment::getRootGUIElement()->getElementFromPoint(mousePos);
	\return Pointer to the element under the mouse. */
	virtual IGUIElement* getHovered() const = 0;

	//! Removes the focus from an element.
	/** Causes a EGET_ELEMENT_FOCUS_LOST event. If the event is absorbed
	then the focus will not be changed.
	\param element Pointer to the element which shall lose the focus.
	\return True on success, false on failure */
	virtual bool removeFocus(IGUIElement* element) = 0;

	//! Returns whether the element has focus
	/** \param element Pointer to the element which is tested.
	\return True if the element has focus, else false. */
	virtual bool hasFocus(IGUIElement* element) const = 0;

	//! Returns the current video driver.
	/** \return Pointer to the video driver. */
	virtual video::IVideoDriver* getVideoDriver() const = 0;

	//! Returns the file system.
	/** \return Pointer to the file system. */
	virtual io::IFileSystem* getFileSystem() const = 0;

	//! returns a pointer to the OS operator
	/** \return Pointer to the OS operator. */
	virtual IOSOperator* getOSOperator() const = 0;

	//! Removes all elements from the environment.
	virtual void clear() = 0;

	//! Posts an input event to the environment.
	/** Usually you do not have to
	use this method, it is used by the engine internally.
	\param event The event to post.
	\return True if succeeded, else false. */
	virtual bool postEventFromUser(const SEvent& event) = 0;

	//! This sets a new event receiver for gui events.
	/** Usually you do not have to
	use this method, it is used by the engine internally.
	\param evr Pointer to the new receiver. */
	virtual void setUserEventReceiver(IEventReceiver* evr) = 0;

	//! Returns pointer to the current gui skin.
	/** \return Pointer to the GUI skin. */
	virtual IGUISkin* getSkin() const = 0;

	//! Sets a new GUI Skin
	/** You can use this to change the appearance of the whole GUI
	Environment. You can set one of the built-in skins or implement your
	own class derived from IGUISkin and enable it using this method.
	To set for example the built-in Windows classic skin, use the following
	code:
	\code
	gui::IGUISkin* newskin = environment->createSkin(gui::EGST_WINDOWS_CLASSIC);
	environment->setSkin(newskin);
	newskin->drop();
	\endcode
	\param skin New skin to use.
	*/
	virtual void setSkin(IGUISkin* skin) = 0;

	//! Creates a new GUI Skin based on a template.
	/** Use setSkin() to set the created skin.
	\param type The type of the new skin.
	\return Pointer to the created skin.
	If you no longer need it, you should call IGUISkin::drop().
	See IReferenceCounted::drop() for more information. */
	virtual IGUISkin* createSkin(EGUI_SKIN_TYPE type) = 0;


	//! Creates the image list from the given texture.
	/** \param texture Texture to split into images
	\param imageSize Dimension of each image
	\param useAlphaChannel Flag whether alpha channel of the texture should be honored.
	\return Pointer to the font. Returns 0 if the font could not be loaded.
	This pointer should not be dropped. See IReferenceCounted::drop() for
	more information. */
	virtual IGUIImageList* createImageList( video::ITexture* texture,
					core::dimension2d<s32> imageSize,
					bool useAlphaChannel ) = 0;

	//! Returns pointer to the font with the specified filename.
	/** Loads the font if it was not loaded before.
	\param filename Filename of the Font.
	\return Pointer to the font. Returns 0 if the font could not be loaded.
	This pointer should not be dropped. See IReferenceCounted::drop() for
	more information. */
	virtual IGUIFont* getFont(const io::path& filename) = 0;

	//! Adds an externally loaded font to the font list.
	/** This method allows to attach an already loaded font to the list of
	existing fonts. The font is grabbed if non-null and adding was successful.
	\param name Name the font should be stored as.
	\param font Pointer to font to add.
	\return Pointer to the font stored. This can differ from given parameter if the name previously existed. */
	virtual IGUIFont* addFont(const io::path& name, IGUIFont* font) = 0;

	//! remove loaded font
	virtual void removeFont(IGUIFont* font) = 0;

	//! Returns the default built-in font.
	/** \return Pointer to the default built-in font.
	This pointer should not be dropped. See IReferenceCounted::drop() for
	more information. */
	virtual IGUIFont* getBuiltInFont() const = 0;

	//! Returns pointer to the sprite bank with the specified file name.
	/** Loads the bank if it was not loaded before.
	\param filename Filename of the sprite bank's origin.
	\return Pointer to the sprite bank. Returns 0 if it could not be loaded.
	This pointer should not be dropped. See IReferenceCounted::drop() for more information. */
	virtual IGUISpriteBank* getSpriteBank(const io::path& filename) = 0;

	//! Adds an empty sprite bank to the manager
	/** \param name Name of the new sprite bank.
	\return Pointer to the sprite bank.
	This pointer should not be dropped. See IReferenceCounted::drop() for more information. */
	virtual IGUISpriteBank* addEmptySpriteBank(const io::path& name) = 0;

	//! Returns the root gui element.
	/** This is the first gui element, the (direct or indirect) parent of all
	other gui elements. It is a valid IGUIElement, with dimensions the same
	size as the screen. 
	\return Pointer to the root element of the GUI. The returned pointer
	should not be dropped. See IReferenceCounted::drop() for more
	information. */
	virtual IGUIElement* getRootGUIElement() = 0;

	//! Adds a button element.
	/** \param rectangle Rectangle specifying the borders of the button.
	\param parent Parent gui element of the button.
	\param id Id with which the gui element can be identified.
	\param text Text displayed on the button.
	\param tooltiptext Text displayed in the tooltip.
	\return Pointer to the created button. Returns 0 if an error occurred.
	This pointer should not be dropped. See IReferenceCounted::drop() for
	more information. */
	virtual IGUIButton* addButton(const core::rect<s32>& rectangle,
		IGUIElement* parent=0, s32 id=-1, const wchar_t* text=0, const wchar_t* tooltiptext = 0) = 0;

	//! Adds an empty window element.
	/** \param rectangle Rectangle specifying the borders of the window.
	\param modal Defines if the dialog is modal. This means, that all other
	gui elements which were created before the window cannot be used until
	it is removed.
	\param text Text displayed as the window title.
	\param parent Parent gui element of the window.
	\param id Id with which the gui element can be identified.
	\return Pointer to the created window. Returns 0 if an error occurred.
	This pointer should not be dropped. See IReferenceCounted::drop() for
	more information. */
	virtual IGUIWindow* addWindow(const core::rect<s32>& rectangle, bool modal = false,
		const wchar_t* text=0, IGUIElement* parent=0, s32 id=-1) = 0;

	//! Adds a modal screen.
	/** This control stops its parent's members from being able to receive
	input until its last child is removed, it then deletes itself.
	\param parent Parent gui element of the modal.
	\return Pointer to the created modal. Returns 0 if an error occurred.
	This pointer should not be dropped. See IReferenceCounted::drop() for
	more information. */
	virtual IGUIElement* addModalScreen(IGUIElement* parent) = 0;

	//! Adds a message box.
	/** \param caption Text to be displayed the title of the message box.
	\param text Text to be displayed in the body of the message box.
	\param modal Defines if the dialog is modal. This means, that all other
	gui elements which were created before the message box cannot be used
	until this messagebox is removed.
	\param flags Flags specifying the layout of the message box. For example
	to create a message box with an OK and a CANCEL button on it, set this
	to (EMBF_OK | EMBF_CANCEL).
	\param parent Parent gui element of the message box.
	\param id Id with which the gui element can be identified.
	\param image Optional texture which will be displayed beside the text as an image
	\return Pointer to the created message box. Returns 0 if an error
	occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUIWindow* addMessageBox(const wchar_t* caption, const wchar_t* text=0,
		bool modal = true, s32 flags = EMBF_OK, IGUIElement* parent=0, s32 id=-1, video::ITexture* image=0) = 0;

	//! Adds a scrollbar.
	/** \param horizontal Specifies if the scroll bar is drawn horizontal
	or vertical.
	\param rectangle Rectangle specifying the borders of the scrollbar.
	\param parent Parent gui element of the scroll bar.
	\param id Id to identify the gui element.
	\return Pointer to the created scrollbar. Returns 0 if an error
	occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUIScrollBar* addScrollBar(bool horizontal, const core::rect<s32>& rectangle,
		IGUIElement* parent=0, s32 id=-1) = 0;

	//! Adds an image element.
	/** \param image Image to be displayed.
	\param pos Position of the image. The width and height of the image is
	taken from the image.
	\param useAlphaChannel Sets if the image should use the alpha channel
	of the texture to draw itself.
	\param parent Parent gui element of the image.
	\param id Id to identify the gui element.
	\param text Title text of the image.
	\return Pointer to the created image element. Returns 0 if an error
	occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUIImage* addImage(video::ITexture* image, core::position2d<s32> pos,
		bool useAlphaChannel=true, IGUIElement* parent=0, s32 id=-1, const wchar_t* text=0) = 0;

	//! Adds an image element.
	/** Use IGUIImage::setImage later to set the image to be displayed.
	\param rectangle Rectangle specifying the borders of the image.
	\param parent Parent gui element of the image.
	\param id Id to identify the gui element.
	\param text Title text of the image.
	\param useAlphaChannel Sets if the image should use the alpha channel
	of the texture to draw itself.
	\return Pointer to the created image element. Returns 0 if an error
	occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUIImage* addImage(const core::rect<s32>& rectangle,
		IGUIElement* parent=0, s32 id=-1, const wchar_t* text=0, bool useAlphaChannel=true) = 0;

	//! Adds a checkbox element.
	/** \param checked Define the initial state of the check box.
	\param rectangle Rectangle specifying the borders of the check box.
	\param parent Parent gui element of the check box.
	\param id Id to identify the gui element.
	\param text Title text of the check box.
	\return Pointer to the created check box. Returns 0 if an error
	occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUICheckBox* addCheckBox(bool checked, const core::rect<s32>& rectangle,
		IGUIElement* parent=0, s32 id=-1, const wchar_t* text=0) = 0;

	//! Adds a list box element.
	/** \param rectangle Rectangle specifying the borders of the list box.
	\param parent Parent gui element of the list box.
	\param id Id to identify the gui element.
	\param drawBackground Flag whether the background should be drawn.
	\return Pointer to the created list box. Returns 0 if an error occurred.
	This pointer should not be dropped. See IReferenceCounted::drop() for
	more information. */
	virtual IGUIListBox* addListBox(const core::rect<s32>& rectangle,
		IGUIElement* parent=0, s32 id=-1, bool drawBackground=false) = 0;

	//! Adds a tree view element.
	/** \param rectangle Position and dimension of list box.
	\param parent Parent gui element of the list box.
	\param id Id to identify the gui element.
	\param drawBackground Flag whether the background should be drawn.
	\param scrollBarVertical Flag whether a vertical scrollbar should be used
	\param scrollBarHorizontal Flag whether a horizontal scrollbar should be used
	\return Pointer to the created list box. Returns 0 if an error occurred.
	This pointer should not be dropped. See IReferenceCounted::drop() for
	more information. */
	virtual IGUITreeView* addTreeView(const core::rect<s32>& rectangle,
		IGUIElement* parent=0, s32 id=-1, bool drawBackground=false,
		bool scrollBarVertical = true, bool scrollBarHorizontal = false) = 0;

	//! Adds a mesh viewer. Not 100% implemented yet.
	/** \param rectangle Rectangle specifying the borders of the mesh viewer.
	\param parent Parent gui element of the mesh viewer.
	\param id Id to identify the gui element.
	\param text Title text of the mesh viewer.
	\return Pointer to the created mesh viewer. Returns 0 if an error
	occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUIMeshViewer* addMeshViewer(const core::rect<s32>& rectangle,
			IGUIElement* parent=0, s32 id=-1, const wchar_t* text=0) = 0;

	//! Adds a file open dialog.
	/** \param title Text to be displayed as the title of the dialog.
	\param modal Defines if the dialog is modal. This means, that all other
	gui elements which were created before the message box cannot be used
	until this messagebox is removed.
	\param parent Parent gui element of the dialog.
	\param id Id to identify the gui element.
	\param restoreCWD If set to true, the current workingn directory will be
	restored after the dialog is closed in some way. Otherwise the working
	directory will be the one that the file dialog was last showing.
	\param startDir Optional path for which the file dialog will be opened.
	\return Pointer to the created file open dialog. Returns 0 if an error
	occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUIFileOpenDialog* addFileOpenDialog(const wchar_t* title=0,
		bool modal=true, IGUIElement* parent=0, s32 id=-1,
		bool restoreCWD=false, io::path::char_type* startDir=0) = 0;

	//! Adds a color select dialog.
	/** \param title The title of the dialog.
	\param modal Defines if the dialog is modal. This means, that all other
	gui elements which were created before the dialog cannot be used
	until it is removed.
	\param parent The parent of the dialog.
	\param id The ID of the dialog.
	\return Pointer to the created file open dialog. Returns 0 if an error
	occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUIColorSelectDialog* addColorSelectDialog(const wchar_t* title = 0,
		bool modal=true, IGUIElement* parent=0, s32 id=-1) = 0;

	//! Adds a static text.
	/** \param text Text to be displayed. Can be altered after creation by SetText().
	\param rectangle Rectangle specifying the borders of the static text
	\param border Set to true if the static text should have a 3d border.
	\param wordWrap Enable if the text should wrap into multiple lines.
	\param parent Parent item of the element, e.g. a window.
	\param id The ID of the element.
	\param fillBackground Enable if the background shall be filled.
	Defaults to false.
	\return Pointer to the created static text. Returns 0 if an error
	occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUIStaticText* addStaticText(const wchar_t* text, const core::rect<s32>& rectangle,
		bool border=false, bool wordWrap=true, IGUIElement* parent=0, s32 id=-1,
		bool fillBackground = false) = 0;

	//! Adds an edit box.
	/** Supports unicode input from every keyboard around the world,
	scrolling, copying and pasting (exchanging data with the clipboard
	directly), maximum character amount, marking, and all shortcuts like
	ctrl+X, ctrl+V, ctrl+C, shift+Left, shift+Right, Home, End, and so on.
	\param text Text to be displayed. Can be altered after creation
	by setText().
	\param rectangle Rectangle specifying the borders of the edit box.
	\param border Set to true if the edit box should have a 3d border.
	\param parent Parent item of the element, e.g. a window.
	Set it to 0 to place the edit box directly in the environment.
	\param id The ID of the element.
	\return Pointer to the created edit box. Returns 0 if an error occurred.
	This pointer should not be dropped. See IReferenceCounted::drop() for
	more information. */
	virtual IGUIEditBox* addEditBox(const wchar_t* text, const core::rect<s32>& rectangle,
		bool border=true, IGUIElement* parent=0, s32 id=-1) = 0;

	//! Adds a spin box.
	/** An edit box with up and down buttons
	\param text Text to be displayed. Can be altered after creation by setText().
	\param rectangle Rectangle specifying the borders of the spin box.
	\param border Set to true if the spin box should have a 3d border.
	\param parent Parent item of the element, e.g. a window.
	Set it to 0 to place the spin box directly in the environment.
	\param id The ID of the element.
	\return Pointer to the created spin box. Returns 0 if an error occurred.
	This pointer should not be dropped. See IReferenceCounted::drop() for
	more information. */
	virtual IGUISpinBox* addSpinBox(const wchar_t* text, const core::rect<s32>& rectangle,
		bool border=true,IGUIElement* parent=0, s32 id=-1) = 0;

	//! Adds an element for fading in or out.
	/** \param rectangle Rectangle specifying the borders of the fader.
	If the pointer is NULL, the whole screen is used.
	\param parent Parent item of the element, e.g. a window.
	\param id An identifier for the fader.
	\return Pointer to the created in-out-fader. Returns 0 if an error
	occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUIInOutFader* addInOutFader(const core::rect<s32>* rectangle=0, IGUIElement* parent=0, s32 id=-1) = 0;

	//! Adds a tab control to the environment.
	/** \param rectangle Rectangle specifying the borders of the tab control.
	\param parent Parent item of the element, e.g. a window.
	Set it to 0 to place the tab control directly in the environment.
	\param fillbackground Specifies if the background of the tab control
	should be drawn.
	\param border Specifies if a flat 3d border should be drawn. This is
	usually not necessary unless you place the control directly into
	the environment without a window as parent.
	\param id An identifier for the tab control.
	\return Pointer to the created tab control element. Returns 0 if an
	error occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUITabControl* addTabControl(const core::rect<s32>& rectangle,
		IGUIElement* parent=0, bool fillbackground=false,
		bool border=true, s32 id=-1) = 0;

	//! Adds tab to the environment.
	/** You can use this element to group other elements. This is not used
	for creating tabs on tab controls, please use IGUITabControl::addTab()
	for this instead.
	\param rectangle Rectangle specifying the borders of the tab.
	\param parent Parent item of the element, e.g. a window.
	Set it to 0 to place the tab directly in the environment.
	\param id An identifier for the tab.
	\return Pointer to the created tab. Returns 0 if an
	error occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUITab* addTab(const core::rect<s32>& rectangle,
		IGUIElement* parent=0, s32 id=-1) = 0;

	//! Adds a context menu to the environment.
	/** \param rectangle Rectangle specifying the borders of the menu.
	Note that the menu is resizing itself based on what items you add.
	\param parent Parent item of the element, e.g. a window.
	Set it to 0 to place the menu directly in the environment.
	\param id An identifier for the menu.
	\return Pointer to the created context menu. Returns 0 if an
	error occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUIContextMenu* addContextMenu(const core::rect<s32>& rectangle,
		IGUIElement* parent=0, s32 id=-1) = 0;

	//! Adds a menu to the environment.
	/** This is like the menu you can find on top of most windows in modern
	graphical user interfaces.
	\param parent Parent item of the element, e.g. a window.
	Set it to 0 to place the menu directly in the environment.
	\param id An identifier for the menu.
	\return Pointer to the created menu. Returns 0 if an
	error occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUIContextMenu* addMenu(IGUIElement* parent=0, s32 id=-1) = 0;

	//! Adds a toolbar to the environment.
	/** It is like a menu that is always placed on top of its parent, and
	contains buttons.
	\param parent Parent item of the element, e.g. a window.
	Set it to 0 to place the tool bar directly in the environment.
	\param id An identifier for the tool bar.
	\return Pointer to the created tool bar. Returns 0 if an
	error occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUIToolBar* addToolBar(IGUIElement* parent=0, s32 id=-1) = 0;

	//! Adds a combo box to the environment.
	/** \param rectangle Rectangle specifying the borders of the combo box.
	\param parent Parent item of the element, e.g. a window.
	Set it to 0 to place the combo box directly in the environment.
	\param id An identifier for the combo box.
	\return Pointer to the created combo box. Returns 0 if an
	error occurred. This pointer should not be dropped. See
	IReferenceCounted::drop() for more information. */
	virtual IGUIComboBox* addComboBox(const core::rect<s32>& rectangle,
		IGUIElement* parent=0, s32 id=-1) = 0;

	//! Adds a table to the environment
	/** \param rectangle Rectangle specifying the borders of the table.
	\param parent Parent item of the element, e.g. a window. Set it to 0
	to place the element directly in the environment.
	\param id An identifier for the table.
	\param drawBackground Flag whether the background should be drawn.
	\return Pointer to the created table. Returns 0 if an error occurred.
	This pointer should not be dropped. See IReferenceCounted::drop() for
	more information. */
	virtual IGUITable* addTable(const core::rect<s32>& rectangle,
		IGUIElement* parent=0, s32 id=-1, bool drawBackground=false) =0;

	//! Get the default element factory which can create all built-in elements
	/** \return Pointer to the factory.
	This pointer should not be dropped. See IReferenceCounted::drop() for
	more information. */
	virtual IGUIElementFactory* getDefaultGUIElementFactory() const = 0;

	//! Adds an element factory to the gui environment.
	/** Use this to extend the gui environment with new element types which
	it should be able to create automatically, for example when loading
	data from xml files.
	\param factoryToAdd Pointer to new factory. */
	virtual void registerGUIElementFactory(IGUIElementFactory* factoryToAdd) = 0;

	//! Get amount of registered gui element factories.
	/** \return Amount of registered gui element factories. */
	virtual u32 getRegisteredGUIElementFactoryCount() const = 0;

	//! Get a gui element factory by index
	/** \param index Index of the factory.
	\return Factory at given index, or 0 if no such factory exists. */
	virtual IGUIElementFactory* getGUIElementFactory(u32 index) const = 0;

	//! Adds a GUI element by its name
	/** Each factory is checked if it can create an element of the given
	name. The first match will be created.
	\param elementName Name of the element to be created.
	\param parent Parent of the new element, if not 0.
	\return New GUI element, or 0 if no such element exists. */
	virtual IGUIElement* addGUIElement(const c8* elementName, IGUIElement* parent=0) = 0;

	//! Saves the current gui into a file.
	/** \param filename Name of the file.
	\param start The GUIElement to start with. Root if 0.
	\return True if saving succeeded, else false. */
	virtual bool saveGUI(const io::path& filename, IGUIElement* start=0) = 0;

	//! Saves the current gui into a file.
	/** \param file The file to write to.
	\param start The GUIElement to start with. Root if 0.
	\return True if saving succeeded, else false. */
	virtual bool saveGUI(io::IWriteFile* file, IGUIElement* start=0) = 0;

	//! Loads the gui. Note that the current gui is not cleared before.
	/** When a parent is set the elements will be added below the parent, the parent itself does not deserialize.
	When the file contains skin-settings from the gui-environment those are always serialized into the 
	guienvironment independent	of the parent setting.
	\param filename Name of the file.
	\param parent Parent for the loaded GUI, root if 0.
	\return True if loading succeeded, else false. */
	virtual bool loadGUI(const io::path& filename, IGUIElement* parent=0) = 0;

	//! Loads the gui. Note that the current gui is not cleared before.
	/** When a parent is set the elements will be added below the parent, the parent itself does not deserialize.
	When the file contains skin-settings from the gui-environment those are always serialized into the 
	guienvironment independent	of the parent setting.
	\param file The file to load from.
	\param parent Parent for the loaded GUI, root if 0.
	\return True if loading succeeded, else false. */
	virtual bool loadGUI(io::IReadFile* file, IGUIElement* parent=0) = 0;

	//! Writes attributes of the gui environment
	virtual void serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options=0) const =0;

	//! Reads attributes of the gui environment
	virtual void deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options=0)=0;

	//! writes an element
	virtual void writeGUIElement(io::IXMLWriter* writer, IGUIElement* node) =0;

	//! reads an element
	virtual void readGUIElement(io::IXMLReader* reader, IGUIElement* node) =0;
};


} // end namespace gui
} // end namespace irr

#endif

