#include "Lib_RandomBuffer_System.h"

#if (defined(_WIN32) || defined(_WIN64))

#include <inttypes.h>
#include <stdbool.h>
#include <malloc.h>
#include <windows.h>

bool read_random_bytes(uint32_t len, uint8_t *buf) {
  HCRYPTPROV ctxt;
  if (!(CryptAcquireContext(&ctxt, NULL, NULL, PROV_RSA_FULL,
                            CRYPT_VERIFYCONTEXT))) {
    DWORD error = GetLastError();
    /* printf("Cannot acquire crypto context: 0x%lx\n", error); */
    return false;
  }
  bool pass = true;
  if (!(CryptGenRandom(ctxt, (uint64_t)len, buf))) {
    /* printf("Cannot read random bytes\n"); */
    pass = false;
  }
  CryptReleaseContext(ctxt, 0);
  return pass;
}

#else

/* assume POSIX here */
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>

bool read_random_bytes(uint32_t len, uint8_t *buf) {
#ifdef SYS_getrandom
  ssize_t res = syscall(SYS_getrandom, buf, (size_t)len, 0);
  if (res == -1) {
    return false;
  }
#else // !defined(SYS_getrandom)
  int fd = open("/dev/urandom", O_RDONLY);
  if (fd == -1) {
    return false;
  }
  ssize_t res = read(fd, buf, (uint64_t)len);
  close(fd);
#endif // defined(SYS_getrandom)
  return ((size_t)res == (size_t)len);
}

#endif

// WARNING: this function is deprecated
bool Lib_RandomBuffer_System_randombytes(uint8_t *x, uint32_t len) {
  return read_random_bytes(len, x);
}

void Lib_RandomBuffer_System_crypto_random(uint8_t *x, uint32_t len) {
    while(!read_random_bytes(len, x)) {}
}
