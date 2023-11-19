/*
Minetest
Copyright (C) 2021 hecks

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

#include "png.h"
#include <string>
#include <sstream>
#include <zlib.h>
#include <cassert>
#include "util/serialize.h"
#include "serialization.h"
#include "irrlichttypes.h"

static void writeChunk(std::string &target, const std::string &chunk_str)
{
	assert(chunk_str.size() >= 4);
	assert(chunk_str.size() - 4 < U32_MAX);
	u8 tmp[4];
	target.reserve(target.size() + 4 + chunk_str.size() + 4);

	writeU32(tmp, chunk_str.size() - 4); // Length minus the identifier
	target.append(reinterpret_cast<char*>(tmp), 4);
	target.append(chunk_str); // Data
	const u32 csum = crc32(0, reinterpret_cast<const u8*>(chunk_str.data()),
			chunk_str.size());
	writeU32(tmp, csum); // CRC32 checksum
	target.append(reinterpret_cast<char*>(tmp), 4);
}

std::string encodePNG(const u8 *data, u32 width, u32 height, s32 compression)
{
	std::string file;
	file.append("\x89PNG\r\n\x1a\n");

	{
		std::ostringstream IHDR(std::ios::binary);
		IHDR << "IHDR";
		writeU32(IHDR, width);
		writeU32(IHDR, height);
		// 8 bpp, color type 6 (RGBA)
		IHDR.write("\x08\x06\x00\x00\x00", 5);
		writeChunk(file, IHDR.str());
	}

	{
		std::ostringstream IDAT(std::ios::binary);
		IDAT << "IDAT";
		std::string scanlines;
		scanlines.reserve(width * 4 * height + height);
		for(u32 i = 0; i < height; i++) {
			scanlines.append(1, 0); // Null predictor
			scanlines.append(reinterpret_cast<const char*>(data + width * 4 * i),
					width * 4);
		}
		compressZlib(scanlines, IDAT, compression);
		writeChunk(file, IDAT.str());
	}

	file.append("\x00\x00\x00\x00IEND\xae\x42\x60\x82", 12);

	return file;
}
