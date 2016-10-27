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
#include "content_sao.h"
#include "server.h"

class TestPlayer : public TestBase {
public:
	TestPlayer() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestPlayer"; }

	void runTests(IGameDef *gamedef);

	void testSave(IGameDef *gamedef);
	void testLoad(IGameDef *gamedef);
};

static TestPlayer g_test_instance;

void TestPlayer::runTests(IGameDef *gamedef)
{
	TEST(testSave, gamedef);
	TEST(testLoad, gamedef);
}

void TestPlayer::testSave(IGameDef *gamedef)
{
	RemotePlayer rplayer("testplayer_save", gamedef->idef());
	rplayer.setBreath(10);
	rplayer.hp = 8;
	rplayer.setYaw(0.1f);
	rplayer.setPitch(0.6f);
	rplayer.setPosition(v3f(450.2f, -15.7f, 68.1f));
	rplayer.save(".", gamedef);
	UASSERT(fs::PathExists("testplayer_save"));
}

void TestPlayer::testLoad(IGameDef *gamedef)
{
	RemotePlayer rplayer("testplayer_load", gamedef->idef());
	rplayer.setBreath(10);
	rplayer.hp = 8;
	rplayer.setYaw(0.1f);
	rplayer.setPitch(0.6f);
	rplayer.setPosition(v3f(450.2f, -15.7f, 68.1f));
	rplayer.save(".", gamedef);
	UASSERT(fs::PathExists("testplayer_load"));

	RemotePlayer rplayer_load("testplayer_load", gamedef->idef());
	std::ifstream is("testplayer_load", std::ios_base::binary);
	UASSERT(is.good());
	rplayer_load.deSerialize(is, "testplayer_load");
	is.close();

	UASSERT(strcmp(rplayer_load.getName(), "testplayer_load") == 0);
	UASSERT(rplayer.getBreath() == 10);
	UASSERT(rplayer.hp == 8);
	UASSERT(rplayer.getYaw() == 0.1f);
	UASSERT(rplayer.getPitch() == 0.6f);
	UASSERT(rplayer.getPosition() == v3f(450.2f, -15.7f, 68.1f));
}
