/*
Minetest
Copyright (C) 2010-2016 nerzhul, Loic Blot <loic.blot@unix-experience.fr>

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

#include "exceptions.h"
#include "remoteplayer.h"
#include "server.h"

class TestPlayer : public TestBase
{
public:
	TestPlayer() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestPlayer"; }

	void runTests(IGameDef *gamedef);
};

static TestPlayer g_test_instance;

void TestPlayer::runTests(IGameDef *gamedef)
{
}
