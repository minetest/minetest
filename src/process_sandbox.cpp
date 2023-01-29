/*
Minetest
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

#include "process_sandbox.h"
#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include "filesys.h"
#include "util/basic_macros.h"
#include "util/string.h"
#include <asm/unistd.h>
#include <endian.h>
#include <errno.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <stddef.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <unistd.h>
#elif defined(__APPLE__)
#include "filesys.h"
#include "log.h"
#include "util/string.h"
#include <sandbox.h>
#include <unistd.h>
#endif

#if defined(__linux__) || defined(__APPLE__)

// Closes everything but stdout and stderr.
static void close_resources()
{
#if defined(__linux__)
	std::vector<fs::DirListNode> fds = fs::GetDirListing("/proc/self/fd");
#else
	std::vector<fs::DirListNode> fds = fs::GetDirListing("/dev/fd");
#endif
	for (const fs::DirListNode &node : fds) {
		int fd = stoi(node.name);
		if (fd != STDOUT_FILENO && fd != STDERR_FILENO)
			close(fd);
	}
}

#endif // defined(__linux__) || defined(__APPLE__)

#if defined(_WIN32)

bool start_sandbox()
{
	// Set the process to a low integrity level.
	// This mainly just protects against filesystem writes, I think.
	char sid[SECURITY_MAX_SID_SIZE] alignas(SID);
	DWORD sid_size = sizeof(sid);
	if (!CreateWellKnownSid(WinLowLabelSid, nullptr, (PSID)sid, &sid_size))
		return false;
	HANDLE token;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_DEFAULT, &token))
		return false;
	TOKEN_MANDATORY_LABEL label = { { sid, SE_GROUP_INTEGRITY } };
	bool ok = SetTokenInformation(token, TokenIntegrityLevel, &label, sizeof(label));
	CloseHandle(token);
	return ok;
}

#elif defined(__linux__)

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define IS_LE 1
#elif __BYTE_ORDER == __BIG_ENDIAN
#define IS_LE 0
#else
#error Unsupported byte order
#endif

#if IS_LE
#define SYSCALL_ARG(n) (offsetof(seccomp_data, args) + (n) * 8)
#else
#define SYSCALL_ARG(n) (offsetof(seccomp_data, args) + (n) * 8 + 4)
#endif

#if defined(__NR_mmap) || defined(__NR_mmap2)
#	define HAS_MMAP 1
#	if defined(__NR_mmap)
#		define NR_MMAP_1 __NR_mmap
#		if defined(__NR_mmap2)
#			define NR_MMAP_2 __NR_mmap2
#		endif
#	else
#		define NR_MMAP_1 __NR_mmap2
#	endif
#else
#	define HAS_MMAP 0
#endif

bool start_sandbox()
{
	close_resources();

	static const sock_filter filter_instrs[] = {
		// Load architecture
		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, arch)),
		// Check that byte order matches (for argument checks)
		BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K, __AUDIT_ARCH_LE, IS_LE, !IS_LE),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | ENOSYS),
		// Load syscall number
		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, offsetof(seccomp_data, nr)),
		// Allow futex
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_futex, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
		// Allow clock_gettime
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_clock_gettime, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
		// Allow clock_getres
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_clock_getres, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
#if HAS_MMAP
		// Allow mmap/mmap2 with MAP_ANONYMOUS
#if defined(NR_MMAP_2)
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, NR_MMAP_2, 1, 0),
#endif
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, NR_MMAP_1, 0, 4),
		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, SYSCALL_ARG(3)),
		BPF_JUMP(BPF_JMP | BPF_JSET | BPF_K, MAP_ANONYMOUS, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | ENOSYS),
#endif
		// Allow mprotect
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_mprotect, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
		// Allow munmap
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_munmap, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
		// Allow mremap
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_mremap, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
		// Allow brk
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_brk, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
#if defined(__NR_time)
		// Allow time
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_time, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
#endif
#if defined(__NR_gettimeofday)
		// Allow gettimeofday
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_gettimeofday, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
#endif
		// Allow sigreturn
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_rt_sigreturn, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
		// Allow exit
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_exit, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
		// Allow exit_group
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_exit_group, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
		// Allow write
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_write, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
		// Allow writev
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_writev, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
		// Deny others
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | ENOSYS),
	};
	static const sock_fprog filter = { ARRLEN(filter_instrs), (sock_filter *)filter_instrs };
	if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0)
		return false;
	if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, (unsigned long)&filter, 0, 0))
		return false;
	return true;
}

#elif defined(__APPLE__)

bool start_sandbox()
{
	close_resources();

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations" // The sandbox API is deprecated.
	char *error;
	if (sandbox_init(kSBXProfilePureComputation, SANDBOX_NAMED, &error)) {
		errorstream << "Sandbox error: " << error << std::endl;
		sandbox_free_error(error);
		return false;
	}
#pragma clang diagnostic pop
	return true;
}

#else

bool start_sandbox()
{
	return false;
}

#endif
