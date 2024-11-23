// SPDX-FileCopyrightText: 2024 Luanti Contributors
//
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "hashing.h"
#include "util/sha1.h"
#include "my_sha256.h"

namespace hashing
{

std::string sha1(std::string_view data)
{
	SHA1 sha1;
	sha1.addBytes(data);
	return sha1.getDigest();
}

std::string sha256(std::string_view data)
{
	std::string digest(SHA256_DIGEST_SIZE, '\000');
	auto src = reinterpret_cast<const uint8_t *>(data.data());
	auto dst = reinterpret_cast<uint8_t *>(digest.data());
	SHA256(src, data.size(), dst);
	return digest;
}

}
