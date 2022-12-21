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

#include "ipc_channel.h"
#include "debug.h"
#include <errno.h>
#include <immintrin.h>
#include <linux/futex.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <utility>

static void busy_wait(int n)
{
	for (int i = 0; i < n; i++)
		_mm_pause();
}

static int futex(std::atomic_uint32_t *uaddr, int futex_op, u32 val,
		const struct timespec *timeout, u32 *uaddr2, u32 val3) noexcept
{
	return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr2, val3);
}

static bool wait(IPCChannelBuffer *buf, const struct timespec *timeout) noexcept
{
	// try busy waiting
	for (int i = 0; i < 100; i++) {
		// posted?
		if (buf->futex.exchange(0) == 1)
			return true; // yes
		busy_wait(40);
	}
	// wait with futex
	while (true) {
		// write 2 to show that we're futexing
		if (buf->futex.exchange(2) == 1) {
			// futex was posted => change 2 to 0 (or 1 to 1)
			buf->futex.fetch_and(1);
			return true;
		}
		int s = futex(&buf->futex, FUTEX_WAIT, 2, timeout, nullptr, 0);
		if (s == -1 && errno != EAGAIN)
			return false;
	}
}

static void post(IPCChannelBuffer *buf) noexcept
{
	if (buf->futex.exchange(1) == 2) {
		int s = futex(&buf->futex, FUTEX_WAKE, 1, nullptr, nullptr, 0);
		if (s == -1)
			FATAL_ERROR("FUTEX_WAKE failed unexpectedly");
	}
}

static struct timespec *set_timespec(struct timespec *ts, int ms)
{
	if (ms < 0)
		return nullptr;
	ts->tv_sec = ms / 1000;
	ts->tv_nsec = ms % 1000 * 1000000L;
	return ts;
}

bool IPCChannelEnd::sendSmall(size_t size, const void *data) noexcept
{
	m_out->size = size;
	memcpy(m_out->data, data, size);
	post(m_out);
	return true;
}

bool IPCChannelEnd::sendLarge(size_t size, const void *data, int timeout_ms) noexcept
{
	struct timespec timeout, *timeoutp = set_timespec(&timeout, timeout_ms);
	m_out->size = size;
	do {
		memcpy(m_out->data, data, IPC_CHANNEL_MSG_SIZE);
		post(m_out);
		if (!wait(m_in, timeoutp))
			return false;
		size -= IPC_CHANNEL_MSG_SIZE;
		data = (u8 *)data + IPC_CHANNEL_MSG_SIZE;
	} while (size > IPC_CHANNEL_MSG_SIZE);
	memcpy(m_out->data, data, size);
	post(m_out);
	return true;
}

bool IPCChannelEnd::recv(int timeout_ms) noexcept
{
	struct timespec timeout, *timeoutp = set_timespec(&timeout, timeout_ms);
	if (!wait(m_in, timeoutp))
		return false;
	size_t size = m_in->size;
	try {
		m_recv.resize(size);
	} catch (...) {
		return false;
	}
	u8 *recv_data = m_recv.data();
	while (size > IPC_CHANNEL_MSG_SIZE) {
		memcpy(recv_data, m_in->data, IPC_CHANNEL_MSG_SIZE);
		size -= IPC_CHANNEL_MSG_SIZE;
		recv_data += IPC_CHANNEL_MSG_SIZE;
		post(m_out);
		if (!wait(m_in, timeoutp))
			return false;
	}
	memcpy(recv_data, m_in->data, size);
	return true;
}
