/*
Minetest
Copyright (C) 2013 Jonathan Neuschäfer <j.neuschaefer@gmx.net>

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

#ifndef HEX_HEADER
#define HEX_HEADER

#include <string>

static const char hex_chars[] = "0123456789abcdef";

static inline std::string hex_encode(const char *data, unsigned int data_size)
{
	std::string ret;
	char buf2[3];
	buf2[2] = '\0';

	for(unsigned int i = 0; i < data_size; i++)
	{
		unsigned char c = (unsigned char) data[i];
		buf2[0] = hex_chars[(c & 0xf0) >> 4];
		buf2[1] = hex_chars[c & 0x0f];
		ret.append(buf2);
	}

	return ret;
}

static inline std::string hex_encode(const std::string &data)
{
    return hex_encode(data.c_str(), data.size());
}

#endif
