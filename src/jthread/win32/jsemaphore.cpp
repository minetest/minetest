/*
Minetest
Copyright (C) 2013 sapier, < sapier AT gmx DOT net >

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
#include "jthread/jsemaphore.h"

JSemaphore::JSemaphore() {
	m_hSemaphore = CreateSemaphore(
			0,
			0,
			MAX_SEMAPHORE_COUNT,
			0);
}

JSemaphore::~JSemaphore() {
	CloseHandle(m_hSemaphore);
}

JSemaphore::JSemaphore(int initval) {
	m_hSemaphore = CreateSemaphore(
			0,
			initval,
			MAX_SEMAPHORE_COUNT,
			0);
}

void JSemaphore::Post() {
	ReleaseSemaphore(
			m_hSemaphore,
			1,
			0);
}

void JSemaphore::Wait() {
	WaitForSingleObject(
			m_hSemaphore,
			INFINITE);
}

int JSemaphore::GetValue() {

	long int retval = 0;
	ReleaseSemaphore(
			m_hSemaphore,
			0,
			&retval);

	return retval;
}

