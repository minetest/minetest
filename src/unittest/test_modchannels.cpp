/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "gamedef.h"
#include "modchannels.h"

class TestModChannels : public TestBase
{
public:
	TestModChannels() { TestManager::registerTestModule(this); }
	const char *getName() { return "TestModChannels"; }

	void runTests(IGameDef *gamedef);

	void testJoinChannel(IGameDef *gamedef);
	void testLeaveChannel(IGameDef *gamedef);
	void testSendMessageToChannel(IGameDef *gamedef);
};

static TestModChannels g_test_instance;

void TestModChannels::runTests(IGameDef *gamedef)
{
	TEST(testJoinChannel, gamedef);
	TEST(testLeaveChannel, gamedef);
	TEST(testSendMessageToChannel, gamedef);
}

void TestModChannels::testJoinChannel(IGameDef *gamedef)
{
	// Test join
	UASSERT(gamedef->joinModChannel("test_join_channel"));
	// Test join (fail, already join)
	UASSERT(!gamedef->joinModChannel("test_join_channel"));
}

void TestModChannels::testLeaveChannel(IGameDef *gamedef)
{
	// Test leave (not joined)
	UASSERT(!gamedef->leaveModChannel("test_leave_channel"));

	UASSERT(gamedef->joinModChannel("test_leave_channel"));

	// Test leave (joined)
	UASSERT(gamedef->leaveModChannel("test_leave_channel"));
}

void TestModChannels::testSendMessageToChannel(IGameDef *gamedef)
{
	// Test sendmsg (not joined)
	UASSERT(!gamedef->sendModChannelMessage(
			"test_sendmsg_channel", "testmsgchannel"));

	UASSERT(gamedef->joinModChannel("test_sendmsg_channel"));

	// Test sendmsg (joined)
	UASSERT(gamedef->sendModChannelMessage("test_sendmsg_channel", "testmsgchannel"));
}
