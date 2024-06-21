/*
This file is part of Freeminer.

Freeminer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Freeminer  is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Freeminer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "log_types.h"
#include "convert_json.h"
#include "irr_v3d.h"
#include "network/address.h"

std::ostream &operator<<(std::ostream &s, const v2s16 &p)
{
	s << "(" << p.X << "," << p.Y << ")";
	return s;
}

std::ostream &operator<<(std::ostream &s, const v2s32 &p)
{
	s << "(" << p.X << "," << p.Y << ")";
	return s;
}

std::ostream &operator<<(std::ostream &s, const v2f &p)
{
	s << "(" << p.X << "," << p.Y << ")";
	return s;
}

std::ostream &operator<<(std::ostream &s, const v3pos_t &p)
{
	s << "(" << p.X << "," << p.Y << "," << p.Z << ")";
	return s;
}

std::ostream &operator<<(std::ostream &s, const v3f &p)
{
	s << "(" << p.X << "," << p.Y << "," << p.Z << ")";
	return s;
}

#if USE_OPOS64
std::ostream &operator<<(std::ostream &s, const v3opos_t &p)
{
	s << "(" << p.X << "," << p.Y << "," << p.Z << ")";
	return s;
}
#endif

std::ostream &operator<<(std::ostream &s, const std::map<v3pos_t, unsigned int> &p)
{
	for (auto &i : p)
		s << i.first << "=" << i.second << " ";
	return s;
}

std::ostream &operator<<(std::ostream &s, const irr::video::SColor &c)
{
	s << "c32(" << c.color << ": a=" << c.getAlpha() << ",r=" << c.getRed()
	  << ",g=" << c.getGreen() << ",b=" << c.getBlue() << ")";
	return s;
}

std::ostream &operator<<(std::ostream &s, const irr::video::SColorf &c)
{
	s << "cf32("
	  << "a=" << c.getAlpha() << ",r=" << c.getRed() << ",g=" << c.getGreen()
	  << ",b=" << c.getBlue() << ")";
	return s;
}

#include "util/string.h"
std::ostream &operator<<(std::ostream &s, const std::wstring &w)
{
	s << wide_to_utf8(w);
	return s;
}

std::ostream &operator<<(std::ostream &s, const Json::Value &json)
{
	s << fastWriteJson(json);
	return s;
}

std::ostream &operator<<(std::ostream &s, const Address &addr)
{
	addr.print(s);
	// s << addr.getPort();
	return s;
}