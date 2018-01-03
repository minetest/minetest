/*
Minetest
Copyright (C) 2018 nerzhul, Loic BLOT <loic.blot@unix-experience.fr>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "test.h"

#include "client/gameui.h"

class TestGameUI : public TestBase
{
public:
	TestGameUI() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestGameUI"; }

	void runTests(IGameDef *gamedef);

	void testInit();
	void testFlagSetters();
};

static TestGameUI g_test_instance;

void TestGameUI::runTests(IGameDef *gamedef)
{
	TEST(testInit);
	TEST(testFlagSetters);
}

void TestGameUI::testInit()
{
	GameUI gui{};
	gui.initFlags();
	UASSERT(gui.getFlags().show_chat)
	UASSERT(gui.getFlags().show_hud)

	// @TODO verify if we can create non UI nulldevice to test this function
	gui.init();
}

void TestGameUI::testFlagSetters()
{
	GameUI gui{};
	gui.showMinimap(true);
	UASSERT(gui.getFlags().show_minimap);

	gui.showMinimap(false);
	UASSERT(!gui.getFlags().show_minimap);
}
