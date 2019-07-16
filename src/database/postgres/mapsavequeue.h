/*
Minetest
Copyright (C) 2019 BuckarooBanzai/naturefreshmilk, Thomas Rudin <thomas@rudin.io>

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

#include <string>
#include "database/database.h"
#include "threading/thread.h"
#include "irr_v3d.h"
#include <libpq-fe.h>

// for htonl()
#ifdef _WIN32
        // Without this some of the network functions are not found on mingw
        #ifndef _WIN32_WINNT
                #define _WIN32_WINNT 0x0501
        #endif
        #include <windows.h>
        #include <winsock2.h>
#else
	#include <netinet/in.h>
#endif


struct QueuedItem {
	v3s16 pos;
	std::string data;
};

class MapSaveQueue : public Thread {
public:
	MapSaveQueue(PGconn *m_conn);
	~MapSaveQueue();
	void enqueue(const v3s16 &pos, const std::string &data);

protected:
	void *run();

private:
	PGconn *m_conn = nullptr;
	std::exception_ptr m_async_exception;
	int m_pgversion;
	void save(std::vector<QueuedItem*> *items);
	void saveBlock(QueuedItem *item);

	std::mutex m_mutex;
	std::mutex m_exception_mutex;

	std::vector<QueuedItem*> queue;

};
