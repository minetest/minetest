// Luanti
// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright (C) 2016 sfan5 <sfan5@live.de>

#include "test.h"

#include <string>
#include "exceptions.h"
#include "client/keycode.h"
#include "client/renderingengine.h" // scancode<->keycode conversion

class TestKeycode : public TestBase {
public:
	TestKeycode() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestKeycode"; }

	void runTests(IGameDef *gamedef);

	void testCreateFromString();
	void testCreateFromSKeyInput();
	void testCompare();
};

static TestKeycode g_test_instance;

void TestKeycode::runTests(IGameDef *gamedef)
{
	// TODO: How do we test this without an IrrlichtDevice?
#if 0
	TEST(testCreateFromString);
	TEST(testCreateFromSKeyInput);
	TEST(testCompare);
#endif
}

////////////////////////////////////////////////////////////////////////////////

#define UASSERTEQ_STR(one, two) UASSERT(one == two)
#define UASSERT_HAS_NAME(k) UASSERT(!k.name().empty())

void TestKeycode::testCreateFromString()
{
	KeyPress k;

	k = KeyPress("");
	UASSERTEQ_STR(k.sym(), "");
	UASSERTEQ_STR(k.name(), "");

	// Character key, from char
	k = KeyPress("R");
	UASSERTEQ_STR(k.sym(), "KEY_KEY_R");
	UASSERT_HAS_NAME(k);

	// Character key, from identifier
	k = KeyPress("KEY_KEY_B");
	UASSERTEQ_STR(k.sym(), "KEY_KEY_B");
	UASSERT_HAS_NAME(k);

	// Non-Character key, from identifier
	k = KeyPress("KEY_UP");
	UASSERTEQ_STR(k.sym(), "KEY_UP");
	UASSERT_HAS_NAME(k);

	k = KeyPress("KEY_F6");
	UASSERTEQ_STR(k.sym(), "KEY_F6");
	UASSERT_HAS_NAME(k);

	// Irrlicht-unknown key, from char
	k = KeyPress("/");
	UASSERTEQ_STR(k.sym(), "/");
	UASSERT_HAS_NAME(k);
}

template<typename ...Args>
static u32 toScancode(Args... args)
{
	return RenderingEngine::get_raw_device()->getScancodeFromKey(KeyCode(args...));
}

void TestKeycode::testCreateFromSKeyInput()
{
	KeyPress k;
	irr::SEvent::SKeyInput in;

	// Character key
	in.SystemKeyCode = toScancode(irr::KEY_KEY_3, L'3');
	k = KeyPress(in);
	UASSERTEQ_STR(k.sym(), "KEY_KEY_3");
	UASSERT_HAS_NAME(k);

	// Non-Character key
	in.SystemKeyCode = toScancode(irr::KEY_RSHIFT, L'\0');
	k = KeyPress(in);
	UASSERTEQ_STR(k.sym(), "KEY_RSHIFT");
	UASSERT_HAS_NAME(k);

	// Irrlicht-unknown key
	in.SystemKeyCode = toScancode(KEY_KEY_CODES_COUNT, L'?');
	k = KeyPress(in);
	UASSERTEQ_STR(k.sym(), "?");
	UASSERT_HAS_NAME(k);
}

void TestKeycode::testCompare()
{
	// "Empty" key
	UASSERT(KeyPress() == KeyPress(""));

	// Basic comparison
	UASSERT(KeyPress("5") == KeyPress("KEY_KEY_5"));
	UASSERT(!(KeyPress("5") == KeyPress("KEY_NUMPAD5")));

	// Matching char suffices
	// note: This is a real-world example, Irrlicht maps XK_equal to irr::KEY_PLUS on Linux
	// TODO: Is this still relevant for scancodes?
	irr::SEvent::SKeyInput in;
	/*
	in.Key = irr::KEY_PLUS;
	in.Char = L'=';
	UASSERT(KeyPress("=") == KeyPress(in));
	*/

	// Matching keycode suffices
	irr::SEvent::SKeyInput in2;
	in.SystemKeyCode = toScancode(irr::KEY_OEM_CLEAR, L'\0');
	in2.SystemKeyCode = toScancode(irr::KEY_OEM_CLEAR, L';');
	UASSERT(KeyPress(in) == KeyPress(in2));
}
