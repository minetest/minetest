#include "guiButtonKey.h"
using namespace irr::gui;

GUIButtonKey *GUIButtonKey::addButton(IGUIEnvironment *environment,
		const core::rect<s32> &rectangle, ISimpleTextureSource *tsrc,
		IGUIElement *parent, s32 id, const wchar_t *text,
		const wchar_t *tooltiptext)
{
	auto button = make_irr<GUIButtonKey>(environment,
			parent ? parent : environment->getRootGUIElement(), id, rectangle, tsrc);

	if (text)
		button->setText(text);

	if (tooltiptext)
		button->setToolTipText(tooltiptext);

	return button.get();
}

void GUIButtonKey::setKey(const KeyPress &kp)
{
	key_value = kp;
	keysym = utf8_to_wide(kp.sym());
	super::setText(wstrgettext(kp.name()).c_str());
	if (capturing) {
		capturing = false;
		if (Parent) {
			SEvent e;
			e.EventType = EET_GUI_EVENT;
			e.GUIEvent.Caller = this;
			e.GUIEvent.Element = nullptr;
			e.GUIEvent.EventType = EGET_BUTTON_CLICKED;
			Parent->OnEvent(e);
		}
	}
}

bool GUIButtonKey::OnEvent(const SEvent & event)
{
	switch(event.EventType)
	{
	case EET_KEY_INPUT_EVENT:
		if (!event.KeyInput.PressedDown) {
			if (!capturing || event.KeyInput.Key == KEY_RETURN || event.KeyInput.Key == KEY_SPACE) {
				setPressed(false);
			}
			break;
		} else if (capturing) {
			if (event.KeyInput.Key == KEY_ESCAPE) {
				cancelCapture();
			} else {
				setPressed(true);
				setKey(KeyPress(event.KeyInput));
			}
			return true;
		} else if (event.KeyInput.Key == KEY_RETURN || event.KeyInput.Key == KEY_SPACE) {
			setPressed(true);
			startCapture();
			return true;
		}
		break;
	case EET_MOUSE_INPUT_EVENT:
		if (event.MouseInput.Event == EMIE_LMOUSE_PRESSED_DOWN) {
			if (AbsoluteClippingRect.isPointInside(
						core::position2d<s32>(event.MouseInput.X, event.MouseInput.Y))) {
				Environment->setFocus(this);
				setPressed(true);
				if (capturing)
					cancelCapture();
				else
					startCapture();
				return true;
			}
		} else if (event.MouseInput.Event == EMIE_LMOUSE_LEFT_UP) {
			setPressed(false);
			return true;
		}
		break;
	default:
		break;
	}

	return Parent ? Parent->OnEvent(event) : false;
}
