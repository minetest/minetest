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

#include <server.h>
#include "test.h"

#include "util/string.h"
#include "util/serialize.h"

class FakeServer : public Server
{
public:
	// clang-format off
	FakeServer() : Server("fakeworld", SubgameSpec("fakespec", "fakespec"), true,
					Address(), true, nullptr)
	{
	}
	// clang-format on

private:
	void SendChatMessage(session_t peer_id, const ChatMessage &message)
	{
		// NOOP
	}
};

class TestServerShutdownState : public TestBase
{
public:
	TestServerShutdownState() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestServerShutdownState"; }

	void runTests(IGameDef *gamedef);

	void testInit();
	void testReset();
	void testTrigger();
	void testTick();
};

static TestServerShutdownState g_test_instance;

void TestServerShutdownState::runTests(IGameDef *gamedef)
{
	TEST(testInit);
	TEST(testReset);
	TEST(testTrigger);
	TEST(testTick);
}

void TestServerShutdownState::testInit()
{
	Server::ShutdownState ss;
	UASSERT(!ss.is_requested);
	UASSERT(!ss.should_reconnect);
	UASSERT(ss.message.empty());
	UASSERT(ss.m_timer == 0.0f);
}

void TestServerShutdownState::testReset()
{
	Server::ShutdownState ss;
	ss.reset();
	UASSERT(!ss.is_requested);
	UASSERT(!ss.should_reconnect);
	UASSERT(ss.message.empty());
	UASSERT(ss.m_timer == 0.0f);
}

void TestServerShutdownState::testTrigger()
{
	Server::ShutdownState ss;
	ss.trigger(3.0f, "testtrigger", true);
	UASSERT(!ss.is_requested);
	UASSERT(ss.should_reconnect);
	UASSERT(ss.message == "testtrigger");
	UASSERT(ss.m_timer == 3.0f);
}

void TestServerShutdownState::testTick()
{
	std::unique_ptr<FakeServer> fakeServer(new FakeServer());
	Server::ShutdownState ss;
	ss.trigger(28.0f, "testtrigger", true);
	ss.tick(0.0f, fakeServer.get());

	// Tick with no time should not change anything
	UASSERT(!ss.is_requested);
	UASSERT(ss.should_reconnect);
	UASSERT(ss.message == "testtrigger");
	UASSERT(ss.m_timer == 28.0f);

	// Tick 2 seconds
	ss.tick(2.0f, fakeServer.get());
	UASSERT(!ss.is_requested);
	UASSERT(ss.should_reconnect);
	UASSERT(ss.message == "testtrigger");
	UASSERT(ss.m_timer == 26.0f);

	// Tick remaining seconds + additional expire
	ss.tick(26.1f, fakeServer.get());
	UASSERT(ss.is_requested);
	UASSERT(ss.should_reconnect);
	UASSERT(ss.message == "testtrigger");
	UASSERT(ss.m_timer == 0.0f);
}
