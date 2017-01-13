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
#include "network/socket.h"
#include "settings.h"

class TestSocket : public TestBase {
public:
	TestSocket()
	{
		if (INTERNET_SIMULATOR == false)
			TestManager::registerTestModule(this);
	}

	const char *getName() { return "TestSocket"; }

	void runTests(IGameDef *gamedef);

	void testSocket(int family, const char *fname, bool required);
};

static TestSocket g_test_instance;

void TestSocket::runTests(IGameDef *gamedef)
{
	TEST(testSocket, AF_INET, "IPv4", true);

	if (g_settings->getBool("enable_ipv6"))
		/* Note: Failing to create an IPv6 socket is not technically an
		 * error because the OS may not support IPv6 or it may
		 * have been disabled. IPv6 is not /required/ by
		 * Minetest and therefore this should not cause the unit
		 * test to fail.
		 */
		TEST(testSocket, AF_INET6, "IPv6", false);
}

////////////////////////////////////////////////////////////////////////////////

void TestSocket::testSocket(int family, const char *fname, bool required)
{
	Address address;
	address.setLoopback(family);

	UDPSocket socket;
	if (!socket.init(address)) {
		std::string msg = std::string(fname) +
			" socket creation failed (unit test)";
		if (required) {
			throw SocketException(msg.c_str());
		} else {
			dstream << "WARNING: " << msg << std::endl;
			return;
		}
	}
	// Get real address (OS will have assigned a port for us)
	address = socket.getAddress();

	try {
		const char send_buffer[] = "hello world!";
		socket.send(address, send_buffer, sizeof(send_buffer));

		char recv_buffer[sizeof(send_buffer) + 1] = {0};
		Address sender;
		int received = socket.receive(sender, recv_buffer, sizeof(recv_buffer));
		// FIXME: This fails on some systems
		UASSERT(sizeof(send_buffer) == received);
		UASSERT(memcmp(send_buffer, recv_buffer, sizeof(send_buffer)) == 0);

		UASSERT(sender.serializeHost() == address.serializeHost());
	} catch (SendFailedException &e) {
		if (required)
			throw;
		else
			errorstream << fname << " support enabled but not available!"
				<< std::endl;
	}
}

