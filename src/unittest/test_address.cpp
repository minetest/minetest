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

#include "log.h"
#include "settings.h"
#include "network/socket.h"

class TestAddress : public TestBase
{
public:
	TestAddress() { TestManager::registerTestModule(this); }

	const char *getName() { return "TestAddress"; }

	void runTests(IGameDef *gamedef);

	void testIsLocalhost();
};

static TestAddress g_test_instance;

void TestAddress::runTests(IGameDef *gamedef)
{
	TEST(testIsLocalhost);
}

void TestAddress::testIsLocalhost()
{
	// v4
	UASSERT(Address(127, 0, 0, 1, 0).isLocalhost());
	UASSERT(Address(127, 254, 12, 99, 0).isLocalhost());
	UASSERT(Address(127, 188, 255, 247, 0).isLocalhost());
	UASSERT(!Address(126, 255, 255, 255, 0).isLocalhost());
	UASSERT(!Address(128, 0, 0, 0, 0).isLocalhost());
	UASSERT(!Address(1, 0, 0, 0, 0).isLocalhost());
	UASSERT(!Address(255, 255, 255, 255, 0).isLocalhost());
	UASSERT(!Address(36, 45, 99, 158, 0).isLocalhost());
	UASSERT(!Address(172, 45, 37, 68, 0).isLocalhost());

	// v6
	auto ipv6Bytes = std::make_unique<IPv6AddressBytes>();
	std::vector<u8> ipv6RawAddr = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
	memcpy(ipv6Bytes->bytes, &ipv6RawAddr[0], 16);
	UASSERT(Address(ipv6Bytes.get(), 0).isLocalhost())

	ipv6RawAddr = {16, 34, 0, 0, 0, 0, 29, 0, 0, 0, 188, 0, 0, 0, 0, 14};
	memcpy(ipv6Bytes->bytes, &ipv6RawAddr[0], 16);
	UASSERT(!Address(ipv6Bytes.get(), 0).isLocalhost())
}
