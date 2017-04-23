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

class TestPlayer : public TestBase
{
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
	PlayerSAO sao(NULL, 1, false);
	sao.initialize(&rplayer, std::set<std::string>());
	rplayer.setPlayerSAO(&sao);
	sao.setBreath(10, false);
	sao.setHPRaw(8);
	sao.setYaw(0.1f);
	sao.setPitch(0.6f);
	sao.setBasePosition(v3f(450.2f, -15.7f, 68.1f));
	rplayer.save(".", gamedef);
	UASSERT(fs::PathExists("testplayer_save"));
}

void TestPlayer::testLoad(IGameDef *gamedef)
{
	RemotePlayer rplayer("testplayer_load", gamedef->idef());
	PlayerSAO sao(NULL, 1, false);
	sao.initialize(&rplayer, std::set<std::string>());
	rplayer.setPlayerSAO(&sao);
	sao.setBreath(10, false);
	sao.setHPRaw(8);
	sao.setYaw(0.1f);
	sao.setPitch(0.6f);
	sao.setBasePosition(v3f(450.2f, -15.7f, 68.1f));
	rplayer.save(".", gamedef);
	UASSERT(fs::PathExists("testplayer_load"));

	RemotePlayer rplayer_load("testplayer_load", gamedef->idef());
	PlayerSAO sao_load(NULL, 2, false);
	std::ifstream is("testplayer_load", std::ios_base::binary);
	UASSERT(is.good());
	rplayer_load.deSerialize(is, "testplayer_load", &sao_load);
	is.close();

	UASSERT(strcmp(rplayer_load.getName(), "testplayer_load") == 0);
	UASSERT(sao_load.getBreath() == 10);
	UASSERT(sao_load.getHP() == 8);
	UASSERT(sao_load.getYaw() == 0.1f);
	UASSERT(sao_load.getPitch() == 0.6f);
	UASSERT(sao_load.getBasePosition() == v3f(450.2f, -15.7f, 68.1f));
}
