// SPDX-FileCopyrightText: 2024 Luanti authors
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "catch.h"
#include "threading/ipc_channel.h"
#include <thread>

TEST_CASE("benchmark_ipc_channel")
{
	auto end_a_thread_b_p = make_test_ipc_channel([](IPCChannelEnd end_b) {
		// echos back messages. stops if "" is sent
		while (true) {
			end_b.recv();
			end_b.send(end_b.getRecvData(), end_b.getRecvSize());
			if (end_b.getRecvSize() == 0)
				break;
		}
	});
	// Can't use structured bindings before C++20, because of lamda captures below.
	auto end_a = std::move(end_a_thread_b_p.first);
	auto thread_b = std::move(end_a_thread_b_p.second);

	BENCHMARK("simple_call_1", i) {
		u8 buf[16] = {};
		buf[i & 0xf] = i;
		end_a.exchange(buf, 16);
		return reinterpret_cast<const u8 *>(end_a.getRecvData())[i & 0xf];
	};

	BENCHMARK("simple_call_1000", i) {
		u8 buf[16] = {};
		buf[i & 0xf] = i;
		for (int k = 0; k < 1000; ++k) {
			buf[0] = k & 0xff;
			end_a.exchange(buf, 16);
		}
		return reinterpret_cast<const u8 *>(end_a.getRecvData())[i & 0xf];
	};

	// stop thread_b
	end_a.exchange(nullptr, 0);
	REQUIRE(end_a.getRecvSize() == 0);

	thread_b.join();
}
