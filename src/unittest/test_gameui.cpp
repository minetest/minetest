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
	void testInfoText();
	void testStatusText();
};

static TestGameUI g_test_instance;

void TestGameUI::runTests(IGameDef *gamedef)
{
	TEST(testInit);
	TEST(testFlagSetters);
	TEST(testInfoText);
	TEST(testStatusText);
}

void TestGameUI::testInit()
{
	GameUI gui{};
	// Ensure flags on GameUI init
	UASSERT(gui.getFlags().show_chat)
	UASSERT(gui.getFlags().show_hud)
	UASSERT(!gui.getFlags().show_minimap)
	UASSERT(!gui.getFlags().show_profiler_graph)

	// And after the initFlags init stage
	gui.initFlags();
	UASSERT(gui.getFlags().show_chat)
	UASSERT(gui.getFlags().show_hud)
	UASSERT(!gui.getFlags().show_minimap)
	UASSERT(!gui.getFlags().show_profiler_graph)

	// @TODO verify if we can create non UI nulldevice to test this function
	// gui.init();
}

void TestGameUI::testFlagSetters()
{
	GameUI gui{};
	gui.showMinimap(true);
	UASSERT(gui.getFlags().show_minimap);

	gui.showMinimap(false);
	UASSERT(!gui.getFlags().show_minimap);
}

void TestGameUI::testStatusText()
{
	GameUI gui{};
	gui.showStatusText(L"test status");

	UASSERT(gui.m_statustext_time == 0.0f);
	UASSERT(gui.m_statustext == L"test status");
}

void TestGameUI::testInfoText()
{
	GameUI gui{};
	gui.setInfoText(L"test info");

	UASSERT(gui.m_infotext == L"test info");
}
