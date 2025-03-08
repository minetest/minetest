// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "guiButton.h"
#include "client/keycode.h"
#include "util/string.h"
#include "gettext.h"

using namespace irr;

class GUIButtonKey : public GUIButton
{
	using super = GUIButton;

public:
	//! Constructor
	GUIButtonKey(gui::IGUIEnvironment *environment, gui::IGUIElement *parent,
			s32 id, core::rect<s32> rectangle, ISimpleTextureSource *tsrc,
			bool noclip = false)
		: GUIButton(environment, parent, id, rectangle, tsrc, noclip) {}

	//! Sets the text for the key field
	virtual void setText(const wchar_t *text) override
	{
		setKey(wide_to_utf8(text).c_str());
	}

	//! Gets the value for the key field
	virtual const wchar_t *getText() const override
	{
		return keysym.c_str();
	}

	//! Do not drop returned handle
	static GUIButtonKey *addButton(gui::IGUIEnvironment *environment,
			const core::rect<s32> &rectangle, ISimpleTextureSource *tsrc,
			IGUIElement *parent, s32 id, const wchar_t *text = L"",
			const wchar_t *tooltiptext = L"");

	//! Called if an event happened
	virtual bool OnEvent(const SEvent &event) override;

	//! Start key capture
	void startCapture()
	{
		capturing = true;
		super::setText(wstrgettext("Press Key").c_str());
	}

	//! Cancel key capture
	void cancelCapture()
	{
		capturing = false;
		super::setText(wstrgettext(key_value.name()).c_str());
	}

	//! Check whether the field is capturing a key
	bool isCapturing() const
	{
		return capturing;
	}

	//! Sets the captured key and stop capturing
	void setKey(const KeyPress &key);

	//! Gets the captured key
	const KeyPress &getKey() const
	{
		return key_value;
	}

private:
	void sendKey();

	bool capturing = false;
	KeyPress key_value = {};
	std::wstring keysym;
};
