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

#ifndef SERVER
// Sets the color of all vertices in the mesh
void setMeshVerticesColor(scene::IMesh* mesh, video::SColor& color)
{
	if(mesh == NULL)
		return;
	
	u16 mc = mesh->getMeshBufferCount();
	for(u16 j=0; j<mc; j++)
	{
		scene::IMeshBuffer *buf = mesh->getMeshBuffer(j);
		video::S3DVertex *vertices = (video::S3DVertex*)buf->getVertices();
		u16 vc = buf->getVertexCount();
		for(u16 i=0; i<vc; i++)
		{
			vertices[i].Color = color;
		}
	}
}
#endif

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
	if(d > range * BS)
		return false;

	// Maximum radius of a block
	f32 block_max_radius = 0.5*1.44*1.44*MAP_BLOCKSIZE*BS;
	
	// If block is (nearly) touching the camera, don't
	// bother validating further (that is, render it anyway)
	if(d > block_max_radius)
	{
		// Cosine of the angle between the camera direction
		// and the block direction (camera_dir is an unit vector)
		f32 cosangle = dforward / d;
		
		// Compensate for the size of the block
		// (as the block has to be shown even if it's a bit off FOV)
		// This is an estimate.
		cosangle += block_max_radius / dforward;

		// If block is not in the field of view, skip it
		if(cosangle < cos(camera_fov / 2))
			return false;
	}

	return true;
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



