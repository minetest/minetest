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

bool JSemaphore::Wait(unsigned int time_ms) {
	unsigned int retval = WaitForSingleObject(
			m_hSemaphore,
			time_ms);

	if (retval == WAIT_OBJECT_0)
	{
		return true;
	}
	else {
		assert(retval == WAIT_TIMEOUT);
		return false;
	}
}

typedef LONG (NTAPI *_NtQuerySemaphore)(
    HANDLE SemaphoreHandle,
    DWORD SemaphoreInformationClass,
    PVOID SemaphoreInformation,
    ULONG SemaphoreInformationLength,
    PULONG ReturnLength OPTIONAL
);

typedef struct _SEMAPHORE_BASIC_INFORMATION {
    ULONG CurrentCount;
    ULONG MaximumCount;
} SEMAPHORE_BASIC_INFORMATION;

/* Note: this will only work as long as jthread is directly linked to application */
/* it's gonna fail if someone tries to build jthread as dll */
static _NtQuerySemaphore NtQuerySemaphore = 
		(_NtQuerySemaphore)
		GetProcAddress 
		(GetModuleHandle ("ntdll.dll"), "NtQuerySemaphore");

int JSemaphore::GetValue() {
	SEMAPHORE_BASIC_INFORMATION BasicInfo;
	LONG retval;

	assert(NtQuerySemaphore);
	
	retval = NtQuerySemaphore (m_hSemaphore, 0,
		&BasicInfo, sizeof (SEMAPHORE_BASIC_INFORMATION), NULL);

	if (retval == ERROR_SUCCESS)
	{
		return BasicInfo.CurrentCount;
	}
	else {
		assert("unable to read semaphore count" == 0);
	}
}

