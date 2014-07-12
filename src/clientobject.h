/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef CLIENTOBJECT_HEADER
#define CLIENTOBJECT_HEADER

#include "irrlichttypes_extrabloated.h"
#include "activeobject.h"
#include <map>

/*

Some planning
-------------

* Client receives a network packet with information of added objects
  in it
* Client supplies the information to its ClientEnvironment
* The environment adds the specified objects to itself

*/

class ClientEnvironment;
class ITextureSource;
class IGameDef;
class LocalPlayer;
struct ItemStack;

class ClientActiveObject : public ActiveObject
{
public:
	ClientActiveObject(u16 id, IGameDef *gamedef, ClientEnvironment *env);
	virtual ~ClientActiveObject();

	virtual void addToScene(scene::ISceneManager *smgr, ITextureSource *tsrc,
			IrrlichtDevice *irr){}
	virtual void removeFromScene(bool permanent){}
	// 0 <= light_at_pos <= LIGHT_SUN
	virtual void updateLight(u8 light_at_pos){}
	virtual v3s16 getLightPosition(){return v3s16(0,0,0);}
	virtual core::aabbox3d<f32>* getSelectionBox(){return NULL;}
	virtual bool getCollisionBox(aabb3f *toset){return false;}
	virtual bool collideWithObjects(){return false;}
	virtual v3f getPosition(){return v3f(0,0,0);}
	virtual scene::IMeshSceneNode *getMeshSceneNode(){return NULL;}
	virtual scene::IAnimatedMeshSceneNode *getAnimatedMeshSceneNode(){return NULL;}
	virtual scene::IBillboardSceneNode *getSpriteSceneNode(){return NULL;}
	virtual bool isPlayer() const {return false;}
	virtual bool isLocalPlayer() const {return false;}
	virtual void setAttachments(){}
	virtual bool doShowSelectionBox(){return true;}
	virtual void updateCameraOffset(v3s16 camera_offset){};

	// Step object in time
	virtual void step(float dtime, ClientEnvironment *env){}

	// Process a message sent by the server side object
	virtual void processMessage(const std::string &data){}

	virtual std::string infoText() {return "";}
	virtual std::string debugInfoText() {return "";}

	/*
		This takes the return value of
		ServerActiveObject::getClientInitializationData
	*/
	virtual void initialize(const std::string &data){}

	// Create a certain type of ClientActiveObject
	static ClientActiveObject* create(u8 type, IGameDef *gamedef,
			ClientEnvironment *env);

	// If returns true, punch will not be sent to the server
	virtual bool directReportPunch(v3f dir, const ItemStack *punchitem=NULL,
			float time_from_last_punch=1000000)
	{ return false; }

protected:
	// Used for creating objects based on type
	typedef ClientActiveObject* (*Factory)(IGameDef *gamedef, ClientEnvironment *env);
	static void registerType(u16 type, Factory f);
	IGameDef *m_gamedef;
	ClientEnvironment *m_env;
private:
	// Used for creating objects based on type
	static std::map<u16, Factory> m_types;
};

struct DistanceSortedActiveObject
{
	ClientActiveObject *obj;
	f32 d;

	DistanceSortedActiveObject(ClientActiveObject *a_obj, f32 a_d)
	{
		obj = a_obj;
		d = a_d;
	}

	bool operator < (const DistanceSortedActiveObject &other) const
	{
		return d < other.d;
	}
};

#endif
