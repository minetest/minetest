/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>
Copyright (C) 2013 Kahrl <kahrl@gmx.net>

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

#include "shader.h"
#include "irrlichttypes_extrabloated.h"
#include "debug.h"
#include "main.h" // for g_settings
#include "filesys.h"
#include "util/container.h"
#include "util/thread.h"
#include "settings.h"
#include <iterator>
#include <ICameraSceneNode.h>
#include <IGPUProgrammingServices.h>
#include <IMaterialRenderer.h>
#include <IMaterialRendererServices.h>
#include <IShaderConstantSetCallBack.h>
#include "EShaderTypes.h"
#include "log.h"
#include "gamedef.h"
#include "strfnd.h" // trim()

/*
	A cache from shader name to shader path
*/
MutexedMap<std::string, std::string> g_shadername_to_path_cache;

/*
	Gets the path to a shader by first checking if the file
	  name_of_shader/filename
	exists in shader_path and if not, using the data path.

	If not found, returns "".

	Utilizes a thread-safe cache.
*/
std::string getShaderPath(const std::string &name_of_shader,
		const std::string &filename)
{
	std::string combined = name_of_shader + DIR_DELIM + filename;
	std::string fullpath = "";
	/*
		Check from cache
	*/
	bool incache = g_shadername_to_path_cache.get(combined, &fullpath);
	if(incache)
		return fullpath;

	/*
		Check from shader_path
	*/
	std::string shader_path = g_settings->get("shader_path");
	if(shader_path != "")
	{
		std::string testpath = shader_path + DIR_DELIM + combined;
		if(fs::PathExists(testpath))
			fullpath = testpath;
	}

	/*
		Check from default data directory
	*/
	if(fullpath == "")
	{
		std::string rel_path = std::string("client") + DIR_DELIM
				+ "shaders" + DIR_DELIM
				+ name_of_shader + DIR_DELIM
				+ filename;
		std::string testpath = porting::path_share + DIR_DELIM + rel_path;
		if(fs::PathExists(testpath))
			fullpath = testpath;
	}

	// Add to cache (also an empty result is cached)
	g_shadername_to_path_cache.set(combined, fullpath);

	// Finally return it
	return fullpath;
}

/*
	SourceShaderCache: A cache used for storing source shaders.
*/

class SourceShaderCache
{
public:
	void insert(const std::string &name_of_shader,
			const std::string &filename,
			const std::string &program,
			bool prefer_local)
	{
		std::string combined = name_of_shader + DIR_DELIM + filename;
		// Try to use local shader instead if asked to
		if(prefer_local){
			std::string path = getShaderPath(name_of_shader, filename);
			if(path != ""){
				std::string p = readFile(path);
				if(p != ""){
					m_programs[combined] = p;
					return;
				}
			}
		}
		m_programs[combined] = program;
	}
	std::string get(const std::string &name_of_shader,
			const std::string &filename)
	{
		std::string combined = name_of_shader + DIR_DELIM + filename;
		std::map<std::string, std::string>::iterator n;
		n = m_programs.find(combined);
		if(n != m_programs.end())
			return n->second;
		return "";
	}
	// Primarily fetches from cache, secondarily tries to read from filesystem
	std::string getOrLoad(const std::string &name_of_shader,
			const std::string &filename)
	{
		std::string combined = name_of_shader + DIR_DELIM + filename;
		std::map<std::string, std::string>::iterator n;
		n = m_programs.find(combined);
		if(n != m_programs.end())
			return n->second;
		std::string path = getShaderPath(name_of_shader, filename);
		if(path == ""){
			infostream<<"SourceShaderCache::getOrLoad(): No path found for \""
					<<combined<<"\""<<std::endl;
			return "";
		}
		infostream<<"SourceShaderCache::getOrLoad(): Loading path \""<<path
				<<"\""<<std::endl;
		std::string p = readFile(path);
		if(p != ""){
			m_programs[combined] = p;
			return p;
		}
		return "";
	}
private:
	std::map<std::string, std::string> m_programs;
	std::string readFile(const std::string &path)
	{
		std::ifstream is(path.c_str(), std::ios::binary);
		if(!is.is_open())
			return "";
		std::ostringstream tmp_os;
		tmp_os << is.rdbuf();
		return tmp_os.str();
	}
};

/*
	ShaderCallback: Sets constants that can be used in shaders
*/

class IShaderConstantSetterRegistry
{
public:
	virtual ~IShaderConstantSetterRegistry(){};
	virtual void onSetConstants(video::IMaterialRendererServices *services,
			bool is_highlevel, const std::string &name) = 0;
};

class ShaderCallback : public video::IShaderConstantSetCallBack
{
	IShaderConstantSetterRegistry *m_scsr;
	std::string m_name;

public:
	ShaderCallback(IShaderConstantSetterRegistry *scsr, const std::string &name):
		m_scsr(scsr),
		m_name(name)
	{}
	~ShaderCallback() {}

	virtual void OnSetConstants(video::IMaterialRendererServices *services, s32 userData)
	{
		video::IVideoDriver *driver = services->getVideoDriver();
		assert(driver);

		bool is_highlevel = userData;

		m_scsr->onSetConstants(services, is_highlevel, m_name);
	}
};

/*
	MainShaderConstantSetter: Set basic constants required for almost everything
*/

class MainShaderConstantSetter : public IShaderConstantSetter
{
public:
	MainShaderConstantSetter(IrrlichtDevice *device)
	{}
	~MainShaderConstantSetter() {}

	virtual void onSetConstants(video::IMaterialRendererServices *services,
			bool is_highlevel)
	{
		video::IVideoDriver *driver = services->getVideoDriver();
		assert(driver);

		// set inverted world matrix
		core::matrix4 invWorld = driver->getTransform(video::ETS_WORLD);
		invWorld.makeInverse();
		if(is_highlevel)
			services->setVertexShaderConstant("mInvWorld", invWorld.pointer(), 16);
		else
			services->setVertexShaderConstant(invWorld.pointer(), 0, 4);

		// set clip matrix
		core::matrix4 worldViewProj;
		worldViewProj = driver->getTransform(video::ETS_PROJECTION);
		worldViewProj *= driver->getTransform(video::ETS_VIEW);
		worldViewProj *= driver->getTransform(video::ETS_WORLD);
		if(is_highlevel)
			services->setVertexShaderConstant("mWorldViewProj", worldViewProj.pointer(), 16);
		else
			services->setVertexShaderConstant(worldViewProj.pointer(), 4, 4);

		// set transposed world matrix
		core::matrix4 transWorld = driver->getTransform(video::ETS_WORLD);
		transWorld = transWorld.getTransposed();
		if(is_highlevel)
			services->setVertexShaderConstant("mTransWorld", transWorld.pointer(), 16);
		else
			services->setVertexShaderConstant(transWorld.pointer(), 8, 4);

		// set world matrix
		core::matrix4 world = driver->getTransform(video::ETS_WORLD);
		if(is_highlevel)
			services->setVertexShaderConstant("mWorld", world.pointer(), 16);
		else
			services->setVertexShaderConstant(world.pointer(), 8, 4);

	}
};

/*
	ShaderSource
*/

class ShaderSource : public IWritableShaderSource, public IShaderConstantSetterRegistry
{
public:
	ShaderSource(IrrlichtDevice *device);
	~ShaderSource();

	/*
		Gets a shader material id from cache or
		- if main thread, from getShaderIdDirect
		- if other thread, adds to request queue and waits for main thread
	*/
	u32 getShaderId(const std::string &name);

	/*
		- If shader material specified by name is found from cache,
		  return the cached id.
		- Otherwise generate the shader material, add to cache and return id.

		The id 0 points to a null shader. Its material is EMT_SOLID.
	*/
	u32 getShaderIdDirect(const std::string &name);

	// Finds out the name of a cached shader.
	std::string getShaderName(u32 id);

	/*
		If shader specified by the name pointed by the id doesn't
		exist, create it, then return the cached shader.

		Can be called from any thread. If called from some other thread
		and not found in cache, the call is queued to the main thread
		for processing.
	*/
	ShaderInfo getShader(u32 id);

	ShaderInfo getShader(const std::string &name)
	{
		return getShader(getShaderId(name));
	}

	// Processes queued shader requests from other threads.
	// Shall be called from the main thread.
	void processQueue();

	// Insert a shader program into the cache without touching the
	// filesystem. Shall be called from the main thread.
	void insertSourceShader(const std::string &name_of_shader,
		const std::string &filename, const std::string &program);

	// Rebuild shaders from the current set of source shaders
	// Shall be called from the main thread.
	void rebuildShaders();

	void addGlobalConstantSetter(IShaderConstantSetter *setter)
	{
		m_global_setters.push_back(setter);
	}

	void onSetConstants(video::IMaterialRendererServices *services,
			bool is_highlevel, const std::string &name);

private:

	// The id of the thread that is allowed to use irrlicht directly
	threadid_t m_main_thread;
	// The irrlicht device
	IrrlichtDevice *m_device;
	// The set-constants callback
	ShaderCallback *m_shader_callback;

	// Cache of source shaders
	// This should be only accessed from the main thread
	SourceShaderCache m_sourcecache;

	// A shader id is index in this array.
	// The first position contains a dummy shader.
	std::vector<ShaderInfo> m_shaderinfo_cache;
	// Maps a shader name to an index in the former.
	std::map<std::string, u32> m_name_to_id;
	// The two former containers are behind this mutex
	JMutex m_shaderinfo_cache_mutex;

	// Queued shader fetches (to be processed by the main thread)
	RequestQueue<std::string, u32, u8, u8> m_get_shader_queue;

	// Global constant setters
	// TODO: Delete these in the destructor
	std::vector<IShaderConstantSetter*> m_global_setters;
};

IWritableShaderSource* createShaderSource(IrrlichtDevice *device)
{
	return new ShaderSource(device);
}

/*
	Generate shader given the shader name.
*/
ShaderInfo generate_shader(std::string name, IrrlichtDevice *device,
		video::IShaderConstantSetCallBack *callback,
		SourceShaderCache *sourcecache);

/*
	Load shader programs
*/
void load_shaders(std::string name, SourceShaderCache *sourcecache,
		video::E_DRIVER_TYPE drivertype, bool enable_shaders,
		std::string &vertex_program, std::string &pixel_program,
		std::string &geometry_program, bool &is_highlevel);

ShaderSource::ShaderSource(IrrlichtDevice *device):
		m_device(device)
{
	assert(m_device);

	m_shader_callback = new ShaderCallback(this, "default");

	m_main_thread = get_current_thread_id();

	// Add a dummy ShaderInfo as the first index, named ""
	m_shaderinfo_cache.push_back(ShaderInfo());
	m_name_to_id[""] = 0;

	// Add main global constant setter
	addGlobalConstantSetter(new MainShaderConstantSetter(device));
}

ShaderSource::~ShaderSource()
{
	//m_shader_callback->drop();

	for (std::vector<IShaderConstantSetter*>::iterator iter = m_global_setters.begin();
			iter != m_global_setters.end(); iter++) {
		delete *iter;
	}
	m_global_setters.clear();
}

u32 ShaderSource::getShaderId(const std::string &name)
{
	//infostream<<"getShaderId(): \""<<name<<"\""<<std::endl;

	{
		/*
			See if shader already exists
		*/
		JMutexAutoLock lock(m_shaderinfo_cache_mutex);
		std::map<std::string, u32>::iterator n;
		n = m_name_to_id.find(name);
		if(n != m_name_to_id.end())
			return n->second;
	}

	/*
		Get shader
	*/
	if(get_current_thread_id() == m_main_thread){
		return getShaderIdDirect(name);
	} else {
		/*errorstream<<"getShaderId(): Queued: name=\""<<name<<"\""<<std::endl;*/

		// We're gonna ask the result to be put into here

		static ResultQueue<std::string, u32, u8, u8> result_queue;

		// Throw a request in
		m_get_shader_queue.add(name, 0, 0, &result_queue);

		/* infostream<<"Waiting for shader from main thread, name=\""
				<<name<<"\""<<std::endl;*/

		while(true) {
			GetResult<std::string, u32, u8, u8>
				result = result_queue.pop_frontNoEx();

			if (result.key == name) {
				return result.item;
			}
			else {
				errorstream << "Got shader with invalid name: " << result.key << std::endl;
			}
		}

	}

	infostream<<"getShaderId(): Failed"<<std::endl;

	return 0;
}

/*
	This method generates all the shaders
*/
u32 ShaderSource::getShaderIdDirect(const std::string &name)
{
	//infostream<<"getShaderIdDirect(): name=\""<<name<<"\""<<std::endl;

	// Empty name means shader 0
	if(name == ""){
		infostream<<"getShaderIdDirect(): name is empty"<<std::endl;
		return 0;
	}

	/*
		Calling only allowed from main thread
	*/
	if(get_current_thread_id() != m_main_thread){
		errorstream<<"ShaderSource::getShaderIdDirect() "
				"called not from main thread"<<std::endl;
		return 0;
	}

	/*
		See if shader already exists
	*/
	{
		JMutexAutoLock lock(m_shaderinfo_cache_mutex);

		std::map<std::string, u32>::iterator n;
		n = m_name_to_id.find(name);
		if(n != m_name_to_id.end()){
			/*infostream<<"getShaderIdDirect(): \""<<name
					<<"\" found in cache"<<std::endl;*/
			return n->second;
		}
	}

	/*infostream<<"getShaderIdDirect(): \""<<name
			<<"\" NOT found in cache. Creating it."<<std::endl;*/

	ShaderInfo info = generate_shader(name, m_device,
			m_shader_callback, &m_sourcecache);

	/*
		Add shader to caches (add dummy shaders too)
	*/

	JMutexAutoLock lock(m_shaderinfo_cache_mutex);

	u32 id = m_shaderinfo_cache.size();
	m_shaderinfo_cache.push_back(info);
	m_name_to_id[name] = id;

	/*infostream<<"getShaderIdDirect(): "
			<<"Returning id="<<id<<" for name \""<<name<<"\""<<std::endl;*/

	return id;
}

std::string ShaderSource::getShaderName(u32 id)
{
	JMutexAutoLock lock(m_shaderinfo_cache_mutex);

	if(id >= m_shaderinfo_cache.size()){
		errorstream<<"ShaderSource::getShaderName(): id="<<id
				<<" >= m_shaderinfo_cache.size()="
				<<m_shaderinfo_cache.size()<<std::endl;
		return "";
	}

	return m_shaderinfo_cache[id].name;
}

ShaderInfo ShaderSource::getShader(u32 id)
{
	JMutexAutoLock lock(m_shaderinfo_cache_mutex);

	if(id >= m_shaderinfo_cache.size())
		return ShaderInfo();

	return m_shaderinfo_cache[id];
}

void ShaderSource::processQueue()
{
	/*
		Fetch shaders
	*/
	//NOTE this is only thread safe for ONE consumer thread!
	if(!m_get_shader_queue.empty()){
		GetRequest<std::string, u32, u8, u8>
				request = m_get_shader_queue.pop();

		/**errorstream<<"ShaderSource::processQueue(): "
				<<"got shader request with "
				<<"name=\""<<request.key<<"\""
				<<std::endl;**/

		m_get_shader_queue.pushResult(request,getShaderIdDirect(request.key));
	}
}

void ShaderSource::insertSourceShader(const std::string &name_of_shader,
		const std::string &filename, const std::string &program)
{
	/*infostream<<"ShaderSource::insertSourceShader(): "
			"name_of_shader=\""<<name_of_shader<<"\", "
			"filename=\""<<filename<<"\""<<std::endl;*/

	assert(get_current_thread_id() == m_main_thread);

	m_sourcecache.insert(name_of_shader, filename, program, true);
}

void ShaderSource::rebuildShaders()
{
	JMutexAutoLock lock(m_shaderinfo_cache_mutex);

	/*// Oh well... just clear everything, they'll load sometime.
	m_shaderinfo_cache.clear();
	m_name_to_id.clear();*/

	/*
		FIXME: Old shader materials can't be deleted in Irrlicht,
		or can they?
		(This would be nice to do in the destructor too)
	*/

	// Recreate shaders
	for(u32 i=0; i<m_shaderinfo_cache.size(); i++){
		ShaderInfo *info = &m_shaderinfo_cache[i];
		if(info->name != ""){
			*info = generate_shader(info->name, m_device,
					m_shader_callback, &m_sourcecache);
		}
	}
}

void ShaderSource::onSetConstants(video::IMaterialRendererServices *services,
		bool is_highlevel, const std::string &name)
{
	for(u32 i=0; i<m_global_setters.size(); i++){
		IShaderConstantSetter *setter = m_global_setters[i];
		setter->onSetConstants(services, is_highlevel);
	}
}

ShaderInfo generate_shader(std::string name, IrrlichtDevice *device,
		video::IShaderConstantSetCallBack *callback,
		SourceShaderCache *sourcecache)
{
	/*infostream<<"generate_shader(): "
			"\""<<name<<"\""<<std::endl;*/

	ShaderInfo shaderinfo;
	shaderinfo.name = name;
	shaderinfo.material = video::EMT_SOLID;

	/*
		Get the base material
	*/
	std::string base_material_name =
		trim(sourcecache->getOrLoad(name, "base.txt"));
	for(s32 i = 0; video::sBuiltInMaterialTypeNames[i] != 0; i++){
		if(video::sBuiltInMaterialTypeNames[i] == base_material_name){
			shaderinfo.material = (video::E_MATERIAL_TYPE) i;
			break;
		}
	}

	bool enable_shaders = g_settings->getBool("enable_shaders");
	if(!enable_shaders)
		return shaderinfo;

	video::IVideoDriver* driver = device->getVideoDriver();
	assert(driver);

	video::IGPUProgrammingServices *gpu = driver->getGPUProgrammingServices();
	if(!gpu){
		errorstream<<"generate_shader(): "
				"failed to generate \""<<name<<"\", "
				"GPU programming not supported."
				<<std::endl;
		return shaderinfo;
	}

	// Choose shader language depending on driver type and settings
	// Then load shaders
	std::string vertex_program;
	std::string pixel_program;
	std::string geometry_program;
	bool is_highlevel;
	load_shaders(name, sourcecache, driver->getDriverType(),
			enable_shaders, vertex_program, pixel_program,
			geometry_program, is_highlevel);

	// Check hardware/driver support
	if(vertex_program != "" &&
			!driver->queryFeature(video::EVDF_VERTEX_SHADER_1_1) &&
			!driver->queryFeature(video::EVDF_ARB_VERTEX_PROGRAM_1)){
		infostream<<"generate_shader(): vertex shaders disabled "
				"because of missing driver/hardware support."
				<<std::endl;
		vertex_program = "";
	}
	if(pixel_program != "" &&
			!driver->queryFeature(video::EVDF_PIXEL_SHADER_1_1) &&
			!driver->queryFeature(video::EVDF_ARB_FRAGMENT_PROGRAM_1)){
		infostream<<"generate_shader(): pixel shaders disabled "
				"because of missing driver/hardware support."
				<<std::endl;
		pixel_program = "";
	}
	if(geometry_program != "" &&
			!driver->queryFeature(video::EVDF_GEOMETRY_SHADER)){
		infostream<<"generate_shader(): geometry shaders disabled "
				"because of missing driver/hardware support."
				<<std::endl;
		geometry_program = "";
	}

	// If no shaders are used, don't make a separate material type
	if(vertex_program == "" && pixel_program == "" && geometry_program == "")
		return shaderinfo;

	// Create shaders header
	std::string shaders_header = "#version 120\n";

	if (g_settings->getBool("generate_normalmaps")){
		shaders_header += "#define GENERATE_NORMALMAPS\n";
		shaders_header += "#define NORMALMAPS_STRENGTH ";
		shaders_header += ftos(g_settings->getFloat("normalmaps_strength"));
		shaders_header += "\n";
		float sample_step;
		int smooth = (int)g_settings->getFloat("normalmaps_smooth");
		switch (smooth){
		case 0:
			sample_step = 0.0078125; // 1.0 / 128.0
			break;
		case 1:
			sample_step = 0.00390625; // 1.0 / 256.0
			break;
		case 2:
			sample_step = 0.001953125; // 1.0 / 512.0
			break;
		default:
			sample_step = 0.0078125;
			break;
		}
		shaders_header += "#define SAMPLE_STEP ";
		shaders_header += ftos(sample_step);
		shaders_header += "\n";
	}

	if (g_settings->getBool("enable_bumpmapping"))
		shaders_header += "#define ENABLE_BUMPMAPPING\n";

	if (g_settings->getBool("enable_parallax_occlusion")){
		shaders_header += "#define ENABLE_PARALLAX_OCCLUSION\n";
		shaders_header += "#define PARALLAX_OCCLUSION_SCALE ";
		shaders_header += ftos(g_settings->getFloat("parallax_occlusion_scale"));
		shaders_header += "\n";
		shaders_header += "#define PARALLAX_OCCLUSION_BIAS ";
		shaders_header += ftos(g_settings->getFloat("parallax_occlusion_bias"));
		shaders_header += "\n";
	}

	if (g_settings->getBool("enable_bumpmapping") || g_settings->getBool("enable_parallax_occlusion"))
		shaders_header += "#define USE_NORMALMAPS\n";

	if (g_settings->getBool("enable_waving_water")){
		shaders_header += "#define ENABLE_WAVING_WATER\n";
		shaders_header += "#define WATER_WAVE_HEIGHT ";
		shaders_header += ftos(g_settings->getFloat("water_wave_height"));
		shaders_header += "\n";
		shaders_header += "#define WATER_WAVE_LENGTH ";
		shaders_header += ftos(g_settings->getFloat("water_wave_length"));
		shaders_header += "\n";
		shaders_header += "#define WATER_WAVE_SPEED ";
		shaders_header += ftos(g_settings->getFloat("water_wave_speed"));
		shaders_header += "\n";
	}

	if (g_settings->getBool("enable_waving_leaves"))
		shaders_header += "#define ENABLE_WAVING_LEAVES\n";

	if (g_settings->getBool("enable_waving_plants"))
		shaders_header += "#define ENABLE_WAVING_PLANTS\n";

	if(pixel_program != "")
		pixel_program = shaders_header + pixel_program;
	if(vertex_program != "")
		vertex_program = shaders_header + vertex_program;
	if(geometry_program != "")
		geometry_program = shaders_header + geometry_program;

	// Call addHighLevelShaderMaterial() or addShaderMaterial()
	const c8* vertex_program_ptr = 0;
	const c8* pixel_program_ptr = 0;
	const c8* geometry_program_ptr = 0;
	if(vertex_program != "")
		vertex_program_ptr = vertex_program.c_str();
	if(pixel_program != "")
		pixel_program_ptr = pixel_program.c_str();
	if(geometry_program != "")
		geometry_program_ptr = geometry_program.c_str();
	s32 shadermat = -1;
	if(is_highlevel){
		infostream<<"Compiling high level shaders for "<<name<<std::endl;
		shadermat = gpu->addHighLevelShaderMaterial(
			vertex_program_ptr,   // Vertex shader program
			"vertexMain",         // Vertex shader entry point
			video::EVST_VS_1_1,   // Vertex shader version
			pixel_program_ptr,    // Pixel shader program
			"pixelMain",          // Pixel shader entry point
			video::EPST_PS_1_1,   // Pixel shader version
			geometry_program_ptr, // Geometry shader program
			"geometryMain",       // Geometry shader entry point
			video::EGST_GS_4_0,   // Geometry shader version
			scene::EPT_TRIANGLES,      // Geometry shader input
			scene::EPT_TRIANGLE_STRIP, // Geometry shader output
			0,                         // Support maximum number of vertices
			callback,             // Set-constant callback
			shaderinfo.material,  // Base material
			1                     // Userdata passed to callback
			);

		if(shadermat == -1){
			errorstream<<"generate_shader(): "
					"failed to generate \""<<name<<"\", "
					"addHighLevelShaderMaterial failed."
					<<std::endl;
			return shaderinfo;
		}
	}
	else{
		infostream<<"Compiling assembly shaders for "<<name<<std::endl;
		shadermat = gpu->addShaderMaterial(
			vertex_program_ptr,   // Vertex shader program
			pixel_program_ptr,    // Pixel shader program
			callback,             // Set-constant callback
			shaderinfo.material,  // Base material
			0                     // Userdata passed to callback
			);

		if(shadermat == -1){
			errorstream<<"generate_shader(): "
					"failed to generate \""<<name<<"\", "
					"addShaderMaterial failed."
					<<std::endl;
			return shaderinfo;
		}
	}

	// HACK, TODO: investigate this better
	// Grab the material renderer once more so minetest doesn't crash on exit
	driver->getMaterialRenderer(shadermat)->grab();

	// Apply the newly created material type
	shaderinfo.material = (video::E_MATERIAL_TYPE) shadermat;
	return shaderinfo;
}

void load_shaders(std::string name, SourceShaderCache *sourcecache,
		video::E_DRIVER_TYPE drivertype, bool enable_shaders,
		std::string &vertex_program, std::string &pixel_program,
		std::string &geometry_program, bool &is_highlevel)
{
	vertex_program = "";
	pixel_program = "";
	geometry_program = "";
	is_highlevel = false;

	if(enable_shaders){
		// Look for high level shaders
		if(drivertype == video::EDT_DIRECT3D9){
			// Direct3D 9: HLSL
			// (All shaders in one file)
			vertex_program = sourcecache->getOrLoad(name, "d3d9.hlsl");
			pixel_program = vertex_program;
			geometry_program = vertex_program;
		}
		else if(drivertype == video::EDT_OPENGL){
			// OpenGL: GLSL
			vertex_program = sourcecache->getOrLoad(name, "opengl_vertex.glsl");
			pixel_program = sourcecache->getOrLoad(name, "opengl_fragment.glsl");
			geometry_program = sourcecache->getOrLoad(name, "opengl_geometry.glsl");
		}
		if(vertex_program != "" || pixel_program != "" || geometry_program != ""){
			is_highlevel = true;
			return;
		}
	}

}
