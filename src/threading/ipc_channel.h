/*
Minetest
Copyright (C) 2022 Desour <vorunbekannt75@web.de>
Copyright (C) 2022 TurkeyMcMac, Jude Melton-Houghton <jwmhjwmh@gmail.com>

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

#pragma once

#include "irrlichttypes.h"
#include <atomic>
#include <string>
#include <type_traits>
#include <vector>

/*
	An IPC channel is used for synchronous communication between two processes.
	Sending two messages in succession from one end is not allowed; messages
	must alternate back and forth.

	IPCChannelShared is situated in shared memory and is used by both ends of
	the channel.
*/

#define IPC_CHANNEL_MSG_SIZE 8192U

struct IPCChannelBuffer
{
	std::atomic_uint32_t futex = ATOMIC_VAR_INIT(0);
	size_t size;
	u8 data[IPC_CHANNEL_MSG_SIZE];
};

struct IPCChannelShared
{
	IPCChannelBuffer a;
	IPCChannelBuffer b;
};

class IPCChannelEnd
{
public:
	IPCChannelEnd() = default;

	static IPCChannelEnd makeA(IPCChannelShared *shared)
	{
		return IPCChannelEnd(&shared->a, &shared->b);
	}

	static IPCChannelEnd makeB(IPCChannelShared *shared)
	{
		return IPCChannelEnd(&shared->b, &shared->a);
	}

	// If send, recv, or exchange return false, stop using the channel.

	bool send(size_t size, const void *data, int timeout_ms = -1) noexcept
	{
		if (size <= IPC_CHANNEL_MSG_SIZE) {
			return sendSmall(size, data);
		} else {
			return sendLarge(size, data, timeout_ms);
		}
	}

	bool recv(int timeout_ms = -1) noexcept;

	bool exchange(size_t size, const void *data, int timeout_ms = -1) noexcept
	{
		return send(size, data, timeout_ms) && recv(timeout_ms);
	}

	template<typename T>
	bool exchange(T msg, int timeout_ms = -1) noexcept
	{
		using U = typename std::remove_reference<T>::type;
		static_assert(std::is_trivially_copyable<U>::value,
				"Cannot send value that is not trivially copyable");
		return send(sizeof(U), &msg, timeout_ms) && recv(timeout_ms);
	}

	// Get information about the last received message
	inline size_t getRecvSize() const noexcept { return m_recv.size(); }
	inline const void *getRecvData() const noexcept { return m_recv.data(); }

private:
	IPCChannelEnd(IPCChannelBuffer *in, IPCChannelBuffer *out): m_in(in), m_out(out) {}

	bool sendSmall(size_t size, const void *data) noexcept;

	bool sendLarge(size_t size, const void *data, int timeout_ms) noexcept;

	IPCChannelBuffer *m_in = nullptr;
	IPCChannelBuffer *m_out = nullptr;
	std::vector<u8> m_recv;
};
