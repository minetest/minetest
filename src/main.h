/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#ifndef MAIN_HEADER
#define MAIN_HEADER

#include <string>
extern std::string getTimestamp();
#define DTIME (getTimestamp()+": ")

#include <jmutex.h>

extern JMutex g_range_mutex;
extern s16 g_viewing_range_nodes;
//extern s16 g_actual_viewing_range_nodes;
extern bool g_viewing_range_all;

#include <fstream>

// Debug streams
extern std::ostream *dout_con_ptr;
extern std::ostream *derr_con_ptr;
extern std::ostream *dout_client_ptr;
extern std::ostream *derr_client_ptr;
extern std::ostream *dout_server_ptr;
extern std::ostream *derr_server_ptr;

#define dout_con (*dout_con_ptr)
#define derr_con (*derr_con_ptr)
#define dout_client (*dout_client_ptr)
#define derr_client (*derr_client_ptr)
#define dout_server (*dout_server_ptr)
#define derr_server (*derr_server_ptr)

// TODO: Move somewhere else? materials.h?
// This header is only for MATERIALS_COUNT
#include "mapnode.h"
extern video::SMaterial g_materials[MATERIALS_COUNT];
//extern video::SMaterial g_mesh_materials[3];

extern IrrlichtDevice *g_device;

#endif

