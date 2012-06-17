/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "string.h"

#include "../sha1.h"
#include "../base64.h"

// Get an sha-1 hash of the player's name combined with
// the password entered. That's what the server uses as
// their password. (Exception : if the password field is
// blank, we send a blank password - this is for backwards
// compatibility with password-less players).
std::string translatePassword(std::string playername, std::wstring password)
{
	if(password.length() == 0)
		return "";

	std::string slt = playername + wide_to_narrow(password);
	SHA1 sha1;
	sha1.addBytes(slt.c_str(), slt.length());
	unsigned char *digest = sha1.getDigest();
	std::string pwd = base64_encode(digest, 20);
	free(digest);
	return pwd;
}

