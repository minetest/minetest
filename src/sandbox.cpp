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

#include "sandbox.h"
#include "util/basic_macros.h"

#if defined(__linux__)

#include <asm/unistd.h>
#include <endian.h>
#include <errno.h>
#include <linux/audit.h>
#include <linux/filter.h>
#include <linux/seccomp.h>
#include <stddef.h>
#include <sys/prctl.h>
#include <unistd.h>

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

bool start_sandbox()
{
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
		// Allow mmap/mmap2 if fd is -1
#if defined(__NR_mmap2)
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_mmap2, 1, 0),
#endif
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_mmap, 0, 4),
		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, SYSCALL_ARG(4)),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, (__u32)-1, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | ENOSYS),
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
		// Allow sigreturn
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_rt_sigreturn, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
		// Allow write/writev if fd is stdout or stderr
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_write, 1, 0),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, __NR_writev, 0, 4),
		BPF_STMT(BPF_LD | BPF_W | BPF_ABS, SYSCALL_ARG(0)),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, STDOUT_FILENO, 1, 0),
		BPF_JUMP(BPF_JMP | BPF_JEQ | BPF_K, STDERR_FILENO, 0, 1),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ALLOW),
		BPF_STMT(BPF_RET | BPF_K, SECCOMP_RET_ERRNO | ENOSYS),
	};
	static const sock_fprog filter = { ARRLEN(filter_instrs), (sock_filter *)filter_instrs };
	if (prctl(PR_SET_NO_NEW_PRIVS, 1, 0, 0, 0) < 0)
		return false;
	if (prctl(PR_SET_SECCOMP, SECCOMP_MODE_FILTER, (unsigned long)&filter, 0, 0))
		return false;
	return true;
}

#else

bool start_sandbox()
{
	return false;
}

#endif
