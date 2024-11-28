// SPDX-FileCopyrightText: 2024 Luanti Contributors
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "hashing.h"

#include "config.h"
#if USE_OPENSSL
#include <openssl/sha.h>
#else
#include "util/sha1.h"
#include "my_sha256.h"
#endif

namespace hashing
{

std::string sha1(std::string_view data)
{
#if USE_OPENSSL
	std::string digest(SHA1_DIGEST_SIZE, '\000');
	auto src = reinterpret_cast<const uint8_t *>(data.data());
	auto dst = reinterpret_cast<uint8_t *>(digest.data());
	SHA1(src, data.size(), dst);
	return digest;
#else
	SHA1 sha1;
	sha1.addBytes(data);
	return sha1.getDigest();
#endif
}

std::string sha256(std::string_view data)
{
	// same api in openssl and my_sha256.h
	std::string digest(SHA256_DIGEST_SIZE, '\000');
	auto src = reinterpret_cast<const uint8_t *>(data.data());
	auto dst = reinterpret_cast<uint8_t *>(digest.data());
	SHA256(src, data.size(), dst);
	return digest;
}

}
