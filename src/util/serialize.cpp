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

#include "serialize.h"

#include <sstream>
#include <iomanip>

// Creates a string encoded in JSON format (almost equivalent to a C string literal)
std::string serializeJsonString(const std::string &plain)
{
	std::ostringstream os(std::ios::binary);
	os<<"\"";
	for(size_t i = 0; i < plain.size(); i++)
	{
		char c = plain[i];
		switch(c)
		{
			case '"': os<<"\\\""; break;
			case '\\': os<<"\\\\"; break;
			case '/': os<<"\\/"; break;
			case '\b': os<<"\\b"; break;
			case '\f': os<<"\\f"; break;
			case '\n': os<<"\\n"; break;
			case '\r': os<<"\\r"; break;
			case '\t': os<<"\\t"; break;
			default:
			{
				if(c >= 32 && c <= 126)
				{
					os<<c;
				}
				else
				{
					u32 cnum = (u32) (u8) c;
					os<<"\\u"<<std::hex<<std::setw(4)<<std::setfill('0')<<cnum;
				}
				break;
			}
		}
	}
	os<<"\"";
	return os.str();
}

// Reads a string encoded in JSON format
std::string deSerializeJsonString(std::istream &is)
{
	std::ostringstream os(std::ios::binary);
	char c, c2;

	// Parse initial doublequote
	is >> c;
	if(c != '"')
		throw SerializationError("JSON string must start with doublequote");

	// Parse characters
	for(;;)
	{
		c = is.get();
		if(is.eof())
			throw SerializationError("JSON string ended prematurely");
		if(c == '"')
		{
			return os.str();
		}
		else if(c == '\\')
		{
			c2 = is.get();
			if(is.eof())
				throw SerializationError("JSON string ended prematurely");
			switch(c2)
			{
				default:  os<<c2; break;
				case 'b': os<<'\b'; break;
				case 'f': os<<'\f'; break;
				case 'n': os<<'\n'; break;
				case 'r': os<<'\r'; break;
				case 't': os<<'\t'; break;
				case 'u':
				{
					char hexdigits[4+1];
					is.read(hexdigits, 4);
					if(is.eof())
						throw SerializationError("JSON string ended prematurely");
					hexdigits[4] = 0;
					std::istringstream tmp_is(hexdigits, std::ios::binary);
					int hexnumber;
					tmp_is >> std::hex >> hexnumber;
					os<<((char)hexnumber);
					break;
				}
			}
		}
		else
		{
			os<<c;
		}
	}
	return os.str();
}


