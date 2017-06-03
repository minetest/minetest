#ifndef L_CAMERA_H
#define L_CAMERA_H

#include "l_base.h"

class Camera;

class LuaCamera : public ModApiBase
{
private:
	static const char className[];
	static const luaL_Reg methods[];

	// garbage collector
	static int gc_object(lua_State *L);

	static int l_set_camera_mode(lua_State *L);
	static int l_get_camera_mode(lua_State *L);

	static int l_get_fov(lua_State *L);

	static int l_get_pos(lua_State *L);
	static int l_get_offset(lua_State *L);
	static int l_get_look_dir(lua_State *L);
	static int l_get_look_vertical(lua_State *L);
	static int l_get_look_horizontal(lua_State *L);
	static int l_get_aspect_ratio(lua_State *L);

	Camera *m_camera;

public:
	LuaCamera(Camera *m);
	~LuaCamera() {}

	static void create(lua_State *L, Camera *m);

	static LuaCamera *checkobject(lua_State *L, int narg);
	static Camera *getobject(LuaCamera *ref);
	static Camera *getobject(lua_State *L, int narg);

	static void Register(lua_State *L);
};

#endif // L_CAMERA_H
