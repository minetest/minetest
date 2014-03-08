/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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


/******************************************************************************/
/******************************************************************************/
/* WARNING!!!! do NOT add this header in any include file or any code file    */
/*             not being a script/modapi file!!!!!!!!                         */
/******************************************************************************/
/******************************************************************************/

#ifndef C_CONTENT_H_
#define C_CONTENT_H_

extern "C" {
#include <lua.h>
}

#include <iostream>
#include <map>
#include <vector>

#include "irrlichttypes_bloated.h"
#include "util/string.h"

namespace Json { class Value; }

struct MapNode;
class INodeDefManager;
struct PointedThing;
struct ItemStack;
struct ItemDefinition;
struct ToolCapabilities;
struct ObjectProperties;
struct SimpleSoundSpec;
struct ServerSoundParams;
class Inventory;
struct NodeBox;
struct ContentFeatures;
struct TileDef;
class Server;
struct DigParams;
struct HitParams;
struct EnumString;
struct NoiseParams;
class DecoSchematic;


ContentFeatures    read_content_features         (lua_State *L, int index);
TileDef            read_tiledef                  (lua_State *L, int index);
void               read_soundspec                (lua_State *L, int index,
                                                  SimpleSoundSpec &spec);
NodeBox            read_nodebox                  (lua_State *L, int index);

void               read_server_sound_params      (lua_State *L, int index,
                                                  ServerSoundParams &params);

void          push_dig_params           (lua_State *L,const DigParams &params);
void          push_hit_params           (lua_State *L,const HitParams &params);

ItemStack     read_item                 (lua_State *L, int index, Server* srv);


ToolCapabilities   read_tool_capabilities    (lua_State *L,
                                              int table);
void               push_tool_capabilities    (lua_State *L,
                                              const ToolCapabilities &prop);

ItemDefinition     read_item_definition      (lua_State *L,
                                              int index,
                                              ItemDefinition default_def);
void               read_object_properties    (lua_State *L,
                                              int index,
                                              ObjectProperties *prop);

void               push_inventory_list       (lua_State *L,
                                              Inventory *inv,
                                              const char *name);
void               read_inventory_list       (lua_State *L,
                                              int tableindex,
                                              Inventory *inv,
                                              const char *name,
                                              Server* srv,
                                              int forcesize=-1);

MapNode            readnode                  (lua_State *L,
                                              int index,
                                              INodeDefManager *ndef);
void               pushnode                  (lua_State *L,
                                              const MapNode &n,
                                              INodeDefManager *ndef);

NodeBox            read_nodebox              (lua_State *L, int index);

void               read_groups               (lua_State *L,
                                              int index,
                                              std::map<std::string, int> &result);

//TODO rename to "read_enum_field"
int                getenumfield              (lua_State *L,
                                              int table,
                                              const char *fieldname,
                                              const EnumString *spec,
                                              int default_);

bool               getflagsfield             (lua_State *L, int table,
                                              const char *fieldname,
                                              FlagDesc *flagdesc,
                                              u32 *flags, u32 *flagmask);

bool               read_flags                (lua_State *L, int index,
                                              FlagDesc *flagdesc,
                                              u32 *flags, u32 *flagmask);

u32                read_flags_table          (lua_State *L, int table,
                                              FlagDesc *flagdesc, u32 *flagmask);

void               push_items                (lua_State *L,
                                              const std::vector<ItemStack> &items);

std::vector<ItemStack> read_items            (lua_State *L,
                                              int index,
                                              Server* srv);

void               read_soundspec            (lua_State *L,
                                              int index,
                                              SimpleSoundSpec &spec);


bool               string_to_enum            (const EnumString *spec,
                                              int &result,
                                              const std::string &str);

NoiseParams*       read_noiseparams          (lua_State *L, int index);

bool               read_noiseparams_nc       (lua_State *L, int index,
                                              NoiseParams *np);

bool               read_schematic            (lua_State *L, int index,
                                              DecoSchematic *dschem,
                                              Server *server);

void               luaentity_get             (lua_State *L,u16 id);

bool               push_json_value           (lua_State *L,
                                              const Json::Value &value,
                                              int nullindex);
void               read_json_value           (lua_State *L,
                                              Json::Value &root,
                                              int index,
                                              u8 recursion = 0);

extern struct EnumString es_TileAnimationType[];

#endif /* C_CONTENT_H_ */
