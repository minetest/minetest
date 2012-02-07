/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#include "utility.h"
#include "gettime.h"
#include "sha1.h"
#include "base64.h"
#include "log.h"
#include <iomanip>

TimeTaker::TimeTaker(const char *name, u32 *result)
{
	m_name = name;
	m_result = result;
	m_running = true;
	m_time1 = getTimeMs();
}

u32 TimeTaker::stop(bool quiet)
{
	if(m_running)
	{
		u32 time2 = getTimeMs();
		u32 dtime = time2 - m_time1;
		if(m_result != NULL)
		{
			(*m_result) += dtime;
		}
		else
		{
			if(quiet == false)
				infostream<<m_name<<" took "<<dtime<<"ms"<<std::endl;
		}
		m_running = false;
		return dtime;
	}
	return 0;
}

u32 TimeTaker::getTime()
{
	u32 time2 = getTimeMs();
	u32 dtime = time2 - m_time1;
	return dtime;
}

const v3s16 g_6dirs[6] =
{
	// +right, +top, +back
	v3s16( 0, 0, 1), // back
	v3s16( 0, 1, 0), // top
	v3s16( 1, 0, 0), // right
	v3s16( 0, 0,-1), // front
	v3s16( 0,-1, 0), // bottom
	v3s16(-1, 0, 0) // left
};

const v3s16 g_26dirs[26] =
{
	// +right, +top, +back
	v3s16( 0, 0, 1), // back
	v3s16( 0, 1, 0), // top
	v3s16( 1, 0, 0), // right
	v3s16( 0, 0,-1), // front
	v3s16( 0,-1, 0), // bottom
	v3s16(-1, 0, 0), // left
	// 6
	v3s16(-1, 1, 0), // top left
	v3s16( 1, 1, 0), // top right
	v3s16( 0, 1, 1), // top back
	v3s16( 0, 1,-1), // top front
	v3s16(-1, 0, 1), // back left
	v3s16( 1, 0, 1), // back right
	v3s16(-1, 0,-1), // front left
	v3s16( 1, 0,-1), // front right
	v3s16(-1,-1, 0), // bottom left
	v3s16( 1,-1, 0), // bottom right
	v3s16( 0,-1, 1), // bottom back
	v3s16( 0,-1,-1), // bottom front
	// 18
	v3s16(-1, 1, 1), // top back-left
	v3s16( 1, 1, 1), // top back-right
	v3s16(-1, 1,-1), // top front-left
	v3s16( 1, 1,-1), // top front-right
	v3s16(-1,-1, 1), // bottom back-left
	v3s16( 1,-1, 1), // bottom back-right
	v3s16(-1,-1,-1), // bottom front-left
	v3s16( 1,-1,-1), // bottom front-right
	// 26
};

const v3s16 g_27dirs[27] =
{
	// +right, +top, +back
	v3s16( 0, 0, 1), // back
	v3s16( 0, 1, 0), // top
	v3s16( 1, 0, 0), // right
	v3s16( 0, 0,-1), // front
	v3s16( 0,-1, 0), // bottom
	v3s16(-1, 0, 0), // left
	// 6
	v3s16(-1, 1, 0), // top left
	v3s16( 1, 1, 0), // top right
	v3s16( 0, 1, 1), // top back
	v3s16( 0, 1,-1), // top front
	v3s16(-1, 0, 1), // back left
	v3s16( 1, 0, 1), // back right
	v3s16(-1, 0,-1), // front left
	v3s16( 1, 0,-1), // front right
	v3s16(-1,-1, 0), // bottom left
	v3s16( 1,-1, 0), // bottom right
	v3s16( 0,-1, 1), // bottom back
	v3s16( 0,-1,-1), // bottom front
	// 18
	v3s16(-1, 1, 1), // top back-left
	v3s16( 1, 1, 1), // top back-right
	v3s16(-1, 1,-1), // top front-left
	v3s16( 1, 1,-1), // top front-right
	v3s16(-1,-1, 1), // bottom back-left
	v3s16( 1,-1, 1), // bottom back-right
	v3s16(-1,-1,-1), // bottom front-left
	v3s16( 1,-1,-1), // bottom front-right
	// 26
	v3s16(0,0,0),
};

static unsigned long next = 1;

/* RAND_MAX assumed to be 32767 */
int myrand(void)
{
   next = next * 1103515245 + 12345;
   return((unsigned)(next/65536) % 32768);
}

void mysrand(unsigned seed)
{
   next = seed;
}

int myrand_range(int min, int max)
{
	if(max-min > MYRAND_MAX)
	{
		errorstream<<"WARNING: myrand_range: max-min > MYRAND_MAX"<<std::endl;
		assert(0);
	}
	if(min > max)
	{
		assert(0);
		return max;
	}
	return (myrand()%(max-min+1))+min;
}

/*
	blockpos: position of block in block coordinates
	camera_pos: position of camera in nodes
	camera_dir: an unit vector pointing to camera direction
	range: viewing range
*/
bool isBlockInSight(v3s16 blockpos_b, v3f camera_pos, v3f camera_dir,
		f32 camera_fov, f32 range, f32 *distance_ptr)
{
	v3s16 blockpos_nodes = blockpos_b * MAP_BLOCKSIZE;
	
	// Block center position
	v3f blockpos(
			((float)blockpos_nodes.X + MAP_BLOCKSIZE/2) * BS,
			((float)blockpos_nodes.Y + MAP_BLOCKSIZE/2) * BS,
			((float)blockpos_nodes.Z + MAP_BLOCKSIZE/2) * BS
	);

	// Block position relative to camera
	v3f blockpos_relative = blockpos - camera_pos;

	// Distance in camera direction (+=front, -=back)
	f32 dforward = blockpos_relative.dotProduct(camera_dir);

	// Total distance
	f32 d = blockpos_relative.getLength();

	if(distance_ptr)
		*distance_ptr = d;
	
	// If block is very close, it is always in sight
	if(d < 1.44*1.44*MAP_BLOCKSIZE*BS/2)
		return true;

	// If block is far away, it's not in sight
	if(d > range)
		return false;

	// Maximum radius of a block
	f32 block_max_radius = 0.5*1.44*1.44*MAP_BLOCKSIZE*BS;
	
	// If block is (nearly) touching the camera, don't
	// bother validating further (that is, render it anyway)
	if(d < block_max_radius)
		return true;
	
	// Cosine of the angle between the camera direction
	// and the block direction (camera_dir is an unit vector)
	f32 cosangle = dforward / d;
	
	// Compensate for the size of the block
	// (as the block has to be shown even if it's a bit off FOV)
	// This is an estimate, plus an arbitary factor
	cosangle += block_max_radius / d * 0.5;

	// If block is not in the field of view, skip it
	if(cosangle < cos(camera_fov / 2))
		return false;

	return true;
}

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



PointedThing::PointedThing():
	type(POINTEDTHING_NOTHING),
	node_undersurface(0,0,0),
	node_abovesurface(0,0,0),
	object_id(-1)
{}

std::string PointedThing::dump() const
{
	std::ostringstream os(std::ios::binary);
	if(type == POINTEDTHING_NOTHING)
	{
		os<<"[nothing]";
	}
	else if(type == POINTEDTHING_NODE)
	{
		const v3s16 &u = node_undersurface;
		const v3s16 &a = node_abovesurface;
		os<<"[node under="<<u.X<<","<<u.Y<<","<<u.Z
			<< " above="<<a.X<<","<<a.Y<<","<<a.Z<<"]";
	}
	else if(type == POINTEDTHING_OBJECT)
	{
		os<<"[object "<<object_id<<"]";
	}
	else
	{
		os<<"[unknown PointedThing]";
	}
	return os.str();
}

void PointedThing::serialize(std::ostream &os) const
{
	writeU8(os, 0); // version
	writeU8(os, (u8)type);
	if(type == POINTEDTHING_NOTHING)
	{
		// nothing
	}
	else if(type == POINTEDTHING_NODE)
	{
		writeV3S16(os, node_undersurface);
		writeV3S16(os, node_abovesurface);
	}
	else if(type == POINTEDTHING_OBJECT)
	{
		writeS16(os, object_id);
	}
}

void PointedThing::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version != 0) throw SerializationError(
			"unsupported PointedThing version");
	type = (PointedThingType) readU8(is);
	if(type == POINTEDTHING_NOTHING)
	{
		// nothing
	}
	else if(type == POINTEDTHING_NODE)
	{
		node_undersurface = readV3S16(is);
		node_abovesurface = readV3S16(is);
	}
	else if(type == POINTEDTHING_OBJECT)
	{
		object_id = readS16(is);
	}
	else
	{
		throw SerializationError(
			"unsupported PointedThingType");
	}
}

bool PointedThing::operator==(const PointedThing &pt2) const
{
	if(type != pt2.type)
		return false;
	if(type == POINTEDTHING_NODE)
	{
		if(node_undersurface != pt2.node_undersurface)
			return false;
		if(node_abovesurface != pt2.node_abovesurface)
			return false;
	}
	else if(type == POINTEDTHING_OBJECT)
	{
		if(object_id != pt2.object_id)
			return false;
	}
	return true;
}

bool PointedThing::operator!=(const PointedThing &pt2) const
{
	return !(*this == pt2);
}
