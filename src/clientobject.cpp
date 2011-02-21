/*
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "clientobject.h"
#include "debug.h"
#include "porting.h"
#include "constants.h"
#include "utility.h"

ClientActiveObject::ClientActiveObject(u16 id):
	ActiveObject(id)
{
}

ClientActiveObject::~ClientActiveObject()
{
	removeFromScene();
}

ClientActiveObject* ClientActiveObject::create(u8 type)
{
	if(type == ACTIVEOBJECT_TYPE_INVALID)
	{
		dstream<<"ClientActiveObject::create(): passed "
				<<"ACTIVEOBJECT_TYPE_INVALID"<<std::endl;
		return NULL;
	}
	else if(type == ACTIVEOBJECT_TYPE_TEST)
	{
		dstream<<"ClientActiveObject::create(): passed "
				<<"ACTIVEOBJECT_TYPE_TEST"<<std::endl;
		return new TestCAO(0);
	}
	else if(type == ACTIVEOBJECT_TYPE_LUA)
	{
		dstream<<"ClientActiveObject::create(): passed "
				<<"ACTIVEOBJECT_TYPE_LUA"<<std::endl;
		return new LuaCAO(0);
	}
	else
	{
		dstream<<"ClientActiveObject::create(): passed "
				<<"unknown type="<<type<<std::endl;
		return NULL;
	}
}

/*
	TestCAO
*/

TestCAO::TestCAO(u16 id):
	ClientActiveObject(id),
	m_node(NULL),
	m_position(v3f(0,10*BS,0))
{
}

TestCAO::~TestCAO()
{
}

void TestCAO::addToScene(scene::ISceneManager *smgr)
{
	if(m_node != NULL)
		return;
	
	video::IVideoDriver* driver = smgr->getVideoDriver();
	
	scene::SMesh *mesh = new scene::SMesh();
	scene::IMeshBuffer *buf = new scene::SMeshBuffer();
	video::SColor c(255,255,255,255);
	video::S3DVertex vertices[4] =
	{
		video::S3DVertex(-BS/2,-BS/4,0, 0,0,0, c, 0,1),
		video::S3DVertex(BS/2,-BS/4,0, 0,0,0, c, 1,1),
		video::S3DVertex(BS/2,BS/4,0, 0,0,0, c, 1,0),
		video::S3DVertex(-BS/2,BS/4,0, 0,0,0, c, 0,0),
	};
	u16 indices[] = {0,1,2,2,3,0};
	buf->append(vertices, 4, indices, 6);
	// Set material
	buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
	buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, false);
	buf->getMaterial().setTexture
			(0, driver->getTexture(porting::getDataPath("rat.png").c_str()));
	buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
	buf->getMaterial().setFlag(video::EMF_FOG_ENABLE, true);
	buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	// Add to mesh
	mesh->addMeshBuffer(buf);
	buf->drop();
	m_node = smgr->addMeshSceneNode(mesh, NULL);
	mesh->drop();
	updateNodePos();
}

void TestCAO::removeFromScene()
{
	if(m_node == NULL)
		return;

	m_node->remove();
	m_node = NULL;
}

void TestCAO::updateLight(u8 light_at_pos)
{
}

v3s16 TestCAO::getLightPosition()
{
	return floatToInt(m_position, BS);
}

void TestCAO::updateNodePos()
{
	if(m_node == NULL)
		return;

	m_node->setPosition(m_position);
	//m_node->setRotation(v3f(0, 45, 0));
}

void TestCAO::step(float dtime)
{
	if(m_node)
	{
		v3f rot = m_node->getRotation();
		//dstream<<"dtime="<<dtime<<", rot.Y="<<rot.Y<<std::endl;
		rot.Y += dtime * 180;
		m_node->setRotation(rot);
	}
}

void TestCAO::processMessage(const std::string &data)
{
	dstream<<"TestCAO: Got data: "<<data<<std::endl;
	std::istringstream is(data, std::ios::binary);
	u16 cmd;
	is>>cmd;
	if(cmd == 0)
	{
		v3f newpos;
		is>>newpos.X;
		is>>newpos.Y;
		is>>newpos.Z;
		m_position = newpos;
		updateNodePos();
	}
}

/*
	LuaCAO
*/

extern "C"{
#include "lstring.h"
}

/*
	Functions for calling from script:

	object_set_position(self, x, y, z)
	object_set_rotation(self, x, y, z)
	object_add_to_mesh(self, image, corners, backface_culling)
	object_clear_mesh(self)

	Callbacks in script:

	step(self, dtime)
	process_message(self, data)
	initialize(self, data)
	TODO:
	string status_text(self)
*/

/*
	object_set_position(self, x, y, z)
*/
static int lf_object_set_position(lua_State *L)
{
	// 4: z
	lua_Number z = lua_tonumber(L, -1);
	lua_pop(L, 1);
	// 3: y
	lua_Number y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	// 2: x
	lua_Number x = lua_tonumber(L, -1);
	lua_pop(L, 1);
	// 1: self
	LuaCAO *self = (LuaCAO*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	
	assert(self);
	
	self->setPosition(v3f(x*BS,y*BS,z*BS));
	
	return 0; // Number of return values
}

/*
	object_set_rotation(self, x, y, z)
*/
static int lf_object_set_rotation(lua_State *L)
{
	// 4: z
	lua_Number z = lua_tonumber(L, -1);
	lua_pop(L, 1);
	// 3: y
	lua_Number y = lua_tonumber(L, -1);
	lua_pop(L, 1);
	// 2: x
	lua_Number x = lua_tonumber(L, -1);
	lua_pop(L, 1);
	// 1: self
	LuaCAO *self = (LuaCAO*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	
	assert(self);
	
	self->setRotation(v3f(x,y,z));
	
	return 0; // Number of return values
}

/*
	object_add_to_mesh(self, image, corners, backface_culling)
	corners is an array like this:
	{{x,y,z},{x,y,z},{x,y,z},{x,y,z}}
*/
static int lf_object_add_to_mesh(lua_State *L)
{
	// 4: backface_culling
	bool backface_culling = lua_toboolean(L, -1);
	lua_pop(L, 1);
	// 3: corners
	if(lua_istable(L, -1) == false)
	{
		dstream<<"ERROR: object_add_to_mesh(): parameter 3 not a table"
				<<std::endl;
		return 0;
	}
	v3f corners[4];
	// Loop table
	for(int i=0; i<4; i++)
	{
		// Get child table
		lua_pushinteger(L, i+1);
		lua_gettable(L, -2);
		if(lua_istable(L, -1) == false)
		{
			dstream<<"ERROR: object_add_to_mesh(): parameter 3 not a"
					" table of tables"<<std::endl;
			return 0;
		}

		// Get x, y and z from the child table
		
		lua_pushinteger(L, 1);
		lua_gettable(L, -2);
		corners[i].X = lua_tonumber(L, -1) * BS;
		lua_pop(L, 1);

		lua_pushinteger(L, 2);
		lua_gettable(L, -2);
		corners[i].Y = lua_tonumber(L, -1) * BS;
		lua_pop(L, 1);

		lua_pushinteger(L, 3);
		lua_gettable(L, -2);
		corners[i].Z = lua_tonumber(L, -1) * BS;
		lua_pop(L, 1);

		// Pop child table
		lua_pop(L, 1);
	}
	lua_pop(L, 1);
	// 2: image
	const char *image = lua_tostring(L, -1);
	lua_pop(L, 1);
	// 1: self
	LuaCAO *self = (LuaCAO*)lua_touserdata(L, -1);
	lua_pop(L, 1);

	assert(self);
	
	self->addToMesh(image, corners, backface_culling);
	
	return 0; // Number of return values
}

/*
	object_clear_mesh(self)
*/
static int lf_object_clear_mesh(lua_State *L)
{
	// 1: self
	LuaCAO *self = (LuaCAO*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	
	assert(self);

	self->clearMesh();

	return 0;
}

LuaCAO::LuaCAO(u16 id):
	ClientActiveObject(id),
	L(NULL),
	m_smgr(NULL),
	m_node(NULL),
	m_mesh(NULL),
	m_position(v3f(0,10*BS,0))
{
	dstream<<"LuaCAO::LuaCAO(): id="<<id<<std::endl;
	L = lua_open();
	assert(L);
	
	// Load libraries
	luaopen_base(L);
	luaopen_table(L);
	luaopen_string(L);
	luaopen_math(L);

	// Disable some stuff
	const char *to_disable[] = {
		"arg",
		"debug",
		"dofile",
		"io",
		"loadfile",
		"os",
		"package",
		"require",
		NULL
	};
	const char **td = to_disable;
	do{
		lua_pushnil(L);
		lua_setglobal(L, *td);
	}while(*(++td));
	
	// Add globals
	//lua_pushlightuserdata(L, this);
	//lua_setglobal(L, "self");
	
	// Register functions
	lua_register(L, "object_set_position", lf_object_set_position);
	lua_register(L, "object_set_rotation", lf_object_set_rotation);
	lua_register(L, "object_add_to_mesh", lf_object_add_to_mesh);
	lua_register(L, "object_clear_mesh", lf_object_clear_mesh);
}

LuaCAO::~LuaCAO()
{
	lua_close(L);
}

void LuaCAO::step(float dtime)
{
	/*
		Call step(self, dtime) from lua
	*/
	
	const char *funcname = "step";
	lua_getglobal(L, funcname);
	if(!lua_isfunction(L,-1))
	{
		lua_pop(L,1);
		dstream<<"WARNING: LuaCAO: Function not found: "
				<<funcname<<std::endl;
		return;
	}
	
	// Parameters:
	// 1: self
	lua_pushlightuserdata(L, this);
	// 2: dtime
	lua_pushnumber(L, dtime);
	
	// Call (2 parameters, 0 result)
	if(lua_pcall(L, 2, 0, 0))
	{
		dstream<<"WARNING: LuaCAO: Error running function "
				<<funcname<<": "
				<<lua_tostring(L,-1)<<std::endl;
		return;
	}
}

void LuaCAO::processMessage(const std::string &data)
{
	/*
		Call process_message(self, data) from lua
	*/
	
	const char *funcname = "process_message";
	lua_getglobal(L, funcname);
	if(!lua_isfunction(L,-1))
	{
		lua_pop(L,1);
		dstream<<"WARNING: LuaCAO: Function not found: "
				<<funcname<<std::endl;
		return;
	}
	
	// Parameters:
	// 1: self
	lua_pushlightuserdata(L, this);
	// 2: data
	lua_pushlstring(L, data.c_str(), data.size());
	
	// Call (2 parameters, 0 results)
	if(lua_pcall(L, 2, 1, 0))
	{
		dstream<<"WARNING: LuaCAO: Error running function "
				<<funcname<<": "
				<<lua_tostring(L,-1)<<std::endl;
		return;
	}
}

void LuaCAO::initialize(const std::string &data)
{
	dstream<<"LuaCAO::initialize(): id="<<getId()<<std::endl;

	std::istringstream is(data, std::ios::binary);
	std::string script = deSerializeLongString(is);
	std::string other = deSerializeLongString(is);

	/*dstream<<"=== script (size="<<script.size()<<")"<<std::endl
			<<script<<std::endl
			<<"==="<<std::endl;*/
	dstream<<"LuaCAO::initialize(): script size="<<script.size()<<std::endl;
	
	/*dstream<<"other.size()="<<other.size()<<std::endl;
	dstream<<"other=\""<<other<<"\""<<std::endl;*/
	
	// Load the script to lua
	loadScript(script);

	/*
		Call initialize(self, data) in the script
	*/
	
	const char *funcname = "initialize";
	lua_getglobal(L, funcname);
	if(!lua_isfunction(L,-1))
	{
		lua_pop(L,1);
		dstream<<"WARNING: LuaCAO: Function not found: "
				<<funcname<<std::endl;
		return;
	}
	
	// Parameters:
	// 1: self
	lua_pushlightuserdata(L, this);
	// 2: data (other)
	lua_pushlstring(L, other.c_str(), other.size());
	
	// Call (2 parameters, 0 result)
	if(lua_pcall(L, 2, 0, 0))
	{
		dstream<<"WARNING: LuaCAO: Error running function "
				<<funcname<<": "
				<<lua_tostring(L,-1)<<std::endl;
		return;
	}

}

void LuaCAO::loadScript(const std::string script)
{
	int ret;
	ret = luaL_loadstring(L, script.c_str());
	if(ret)
	{
		const char *message = lua_tostring(L, -1);
		lua_pop(L, 1);
		dstream<<"LuaCAO::loadScript(): lua_loadstring failed: "
				<<message<<std::endl;
		assert(0);
		return;
	}
	ret = lua_pcall(L, 0, 0, 0);
	if(ret)
	{
		const char *message = lua_tostring(L, -1);
		lua_pop(L, 1);
		dstream<<"LuaCAO::loadScript(): lua_pcall failed: "
				<<message<<std::endl;
		assert(0);
		return;
	}
}

void LuaCAO::addToScene(scene::ISceneManager *smgr)
{
	if(m_smgr != NULL)
	{
		dstream<<"WARNING: LuaCAO::addToScene() called more than once"
				<<std::endl;
		return;
	}

	if(m_node != NULL)
	{
		dstream<<"WARNING: LuaCAO::addToScene(): m_node != NULL"
				<<std::endl;
		return;
	}
	
	m_smgr = smgr;
	
	if(m_mesh == NULL)
	{
		m_mesh = new scene::SMesh();
		m_node = smgr->addMeshSceneNode(m_mesh, NULL);
		
		/*v3f corners[4] = {
			v3f(-BS/2,-BS/4,0),
			v3f(BS/2,-BS/4,0),
			v3f(BS/2,BS/4,0),
			v3f(-BS/2,BS/4,0),
		};
		addToMesh("rat.png", corners, false);*/
	}
	else
	{
		dstream<<"WARNING: LuaCAO::addToScene(): m_mesh != NULL"
				<<std::endl;
		return;
	}
	
	updateNodePos();
}

void LuaCAO::addToMesh(const char *image, v3f *corners, bool backface_culling)
{
	dstream<<"INFO: addToMesh called"<<std::endl;

	video::IVideoDriver* driver = m_smgr->getVideoDriver();
	
	assert(m_mesh);
		
	scene::IMeshBuffer *buf = new scene::SMeshBuffer();
	video::SColor c(255,255,255,255);
	video::S3DVertex vertices[4] =
	{
		video::S3DVertex(corners[0], v3f(0,0,0), c, v2f(0,1)),
		video::S3DVertex(corners[1], v3f(0,0,0), c, v2f(1,1)),
		video::S3DVertex(corners[2], v3f(0,0,0), c, v2f(1,0)),
		video::S3DVertex(corners[3], v3f(0,0,0), c, v2f(0,0)),
		/*video::S3DVertex(-BS/2,-BS/4,0, 0,0,0, c, 0,1),
		video::S3DVertex(BS/2,-BS/4,0, 0,0,0, c, 1,1),
		video::S3DVertex(BS/2,BS/4,0, 0,0,0, c, 1,0),
		video::S3DVertex(-BS/2,BS/4,0, 0,0,0, c, 0,0),*/
	};
	u16 indices[] = {0,1,2,2,3,0};
	buf->append(vertices, 4, indices, 6);
	// Set material
	buf->getMaterial().setFlag(video::EMF_LIGHTING, false);
	buf->getMaterial().setFlag(video::EMF_BACK_FACE_CULLING, backface_culling);
	buf->getMaterial().setTexture
			(0, driver->getTexture(porting::getDataPath(image).c_str()));
	buf->getMaterial().setFlag(video::EMF_BILINEAR_FILTER, false);
	buf->getMaterial().setFlag(video::EMF_FOG_ENABLE, true);
	buf->getMaterial().MaterialType = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
	// Add to mesh
	m_mesh->addMeshBuffer(buf);
	buf->drop();
	// Reset mesh
	if(m_node)
		m_node->setMesh(m_mesh);
}

void LuaCAO::clearMesh()
{
	if(m_node)
	{
		m_node->setMesh(NULL);
	}
	
	if(m_mesh)
	{
		m_mesh->drop();
		m_mesh = NULL;
	}
}

void LuaCAO::removeFromScene()
{
	if(m_node == NULL)
		return;
	
	if(m_node)
	{
		m_node->remove();
		m_node = NULL;
	}
	if(m_mesh)
	{
		m_mesh->drop();
		m_mesh = NULL;
	}

	m_smgr = NULL;
}

void LuaCAO::updateLight(u8 light_at_pos)
{
}

v3s16 LuaCAO::getLightPosition()
{
	return floatToInt(m_position, BS);
}

void LuaCAO::updateNodePos()
{
	if(m_node == NULL)
		return;

	m_node->setPosition(m_position);
	m_node->setRotation(-m_rotation);
}

void LuaCAO::setPosition(v3f pos)
{
	m_position = pos;
	updateNodePos();
}

v3f LuaCAO::getPosition()
{
	return m_position;
}

void LuaCAO::setRotation(v3f rot)
{
	m_rotation = rot;
	updateNodePos();
}

v3f LuaCAO::getRotation()
{
	return m_rotation;
}


