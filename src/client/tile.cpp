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

#include "tile.h"

#include <ICameraSceneNode.h>
#include "util/string.h"
#include "util/container.h"
#include "util/thread.h"
#include "util/numeric.h"
#include "irrlichttypes_extrabloated.h"
#include "debug.h"
#include "filesys.h"
#include "settings.h"
#include "mesh.h"
#include "log.h"
#include "gamedef.h"
#include "util/strfnd.h"
#include "util/string.h" // for parseColorString()
#include "imagefilters.h"
#include "guiscalingfilter.h"
#include "nodedef.h"


#ifdef __ANDROID__
#include <GLES/gl.h>
#endif

/*
	A cache from texture name to texture path
*/
MutexedMap<std::string, std::string> g_texturename_to_path_cache;

/*
	Replaces the filename extension.
	eg:
		std::string image = "a/image.png"
		replace_ext(image, "jpg")
		-> image = "a/image.jpg"
	Returns true on success.
*/
static bool replace_ext(std::string &path, const char *ext)
{
	if (ext == NULL)
		return false;
	// Find place of last dot, fail if \ or / found.
	s32 last_dot_i = -1;
	for (s32 i=path.size()-1; i>=0; i--)
	{
		if (path[i] == '.')
		{
			last_dot_i = i;
			break;
		}

		if (path[i] == '\\' || path[i] == '/')
			break;
	}
	// If not found, return an empty string
	if (last_dot_i == -1)
		return false;
	// Else make the new path
	path = path.substr(0, last_dot_i+1) + ext;
	return true;
}

/*
	Find out the full path of an image by trying different filename
	extensions.

	If failed, return "".
*/
std::string getImagePath(std::string path)
{
	// A NULL-ended list of possible image extensions
	const char *extensions[] = {
		"png", "jpg", "bmp", "tga",
		"pcx", "ppm", "psd", "wal", "rgb",
		NULL
	};
	// If there is no extension, add one
	if (removeStringEnd(path, extensions) == "")
		path = path + ".png";
	// Check paths until something is found to exist
	const char **ext = extensions;
	do{
		bool r = replace_ext(path, *ext);
		if (r == false)
			return "";
		if (fs::PathExists(path))
			return path;
	}
	while((++ext) != NULL);

	return "";
}

/*
	Gets the path to a texture by first checking if the texture exists
	in texture_path and if not, using the data path.

	Checks all supported extensions by replacing the original extension.

	If not found, returns "".

	Utilizes a thread-safe cache.
*/
std::string getTexturePath(const std::string &filename)
{
	std::string fullpath = "";
	/*
		Check from cache
	*/
	bool incache = g_texturename_to_path_cache.get(filename, &fullpath);
	if (incache)
		return fullpath;

	/*
		Check from texture_path
	*/
	const std::string &texture_path = g_settings->get("texture_path");
	if (texture_path != "") {
		std::string testpath = texture_path + DIR_DELIM + filename;
		// Check all filename extensions. Returns "" if not found.
		fullpath = getImagePath(testpath);
	}

	/*
		Check from default data directory
	*/
	if (fullpath == "")
	{
		std::string base_path = porting::path_share + DIR_DELIM + "textures"
				+ DIR_DELIM + "base" + DIR_DELIM + "pack";
		std::string testpath = base_path + DIR_DELIM + filename;
		// Check all filename extensions. Returns "" if not found.
		fullpath = getImagePath(testpath);
	}

	// Add to cache (also an empty result is cached)
	g_texturename_to_path_cache.set(filename, fullpath);

	// Finally return it
	return fullpath;
}

void clearTextureNameCache()
{
	g_texturename_to_path_cache.clear();
}

/*
	Stores internal information about a texture.
*/

struct TextureInfo
{
	std::string name;
	video::ITexture *texture;

	TextureInfo(
			const std::string &name_,
			video::ITexture *texture_=NULL
		):
		name(name_),
		texture(texture_)
	{
	}
};

/*
	SourceImageCache: A cache used for storing source images.
*/

class SourceImageCache
{
public:
	~SourceImageCache() {
		for (std::map<std::string, video::IImage*>::iterator iter = m_images.begin();
				iter != m_images.end(); ++iter) {
			iter->second->drop();
		}
		m_images.clear();
	}
	void insert(const std::string &name, video::IImage *img,
			bool prefer_local, video::IVideoDriver *driver)
	{
		assert(img); // Pre-condition
		// Remove old image
		std::map<std::string, video::IImage*>::iterator n;
		n = m_images.find(name);
		if (n != m_images.end()){
			if (n->second)
				n->second->drop();
		}

		video::IImage* toadd = img;
		bool need_to_grab = true;

		// Try to use local texture instead if asked to
		if (prefer_local){
			std::string path = getTexturePath(name);
			if (path != ""){
				video::IImage *img2 = driver->createImageFromFile(path.c_str());
				if (img2){
					toadd = img2;
					need_to_grab = false;
				}
			}
		}

		if (need_to_grab)
			toadd->grab();
		m_images[name] = toadd;
	}
	video::IImage* get(const std::string &name)
	{
		std::map<std::string, video::IImage*>::iterator n;
		n = m_images.find(name);
		if (n != m_images.end())
			return n->second;
		return NULL;
	}
	// Primarily fetches from cache, secondarily tries to read from filesystem
	video::IImage* getOrLoad(const std::string &name, IrrlichtDevice *device)
	{
		std::map<std::string, video::IImage*>::iterator n;
		n = m_images.find(name);
		if (n != m_images.end()){
			n->second->grab(); // Grab for caller
			return n->second;
		}
		video::IVideoDriver* driver = device->getVideoDriver();
		std::string path = getTexturePath(name);
		if (path == ""){
			infostream<<"SourceImageCache::getOrLoad(): No path found for \""
					<<name<<"\""<<std::endl;
			return NULL;
		}
		infostream<<"SourceImageCache::getOrLoad(): Loading path \""<<path
				<<"\""<<std::endl;
		video::IImage *img = driver->createImageFromFile(path.c_str());

		if (img){
			m_images[name] = img;
			img->grab(); // Grab for caller
		}
		return img;
	}
private:
	std::map<std::string, video::IImage*> m_images;
};

/*
	TextureSource
*/

class TextureSource : public IWritableTextureSource
{
public:
	TextureSource(IrrlichtDevice *device);
	virtual ~TextureSource();

	/*
		Example case:
		Now, assume a texture with the id 1 exists, and has the name
		"stone.png^mineral1".
		Then a random thread calls getTextureId for a texture called
		"stone.png^mineral1^crack0".
		...Now, WTF should happen? Well:
		- getTextureId strips off stuff recursively from the end until
		  the remaining part is found, or nothing is left when
		  something is stripped out

		But it is slow to search for textures by names and modify them
		like that?
		- ContentFeatures is made to contain ids for the basic plain
		  textures
		- Crack textures can be slow by themselves, but the framework
		  must be fast.

		Example case #2:
		- Assume a texture with the id 1 exists, and has the name
		  "stone.png^mineral_coal.png".
		- Now getNodeTile() stumbles upon a node which uses
		  texture id 1, and determines that MATERIAL_FLAG_CRACK
		  must be applied to the tile
		- MapBlockMesh::animate() finds the MATERIAL_FLAG_CRACK and
		  has received the current crack level 0 from the client. It
		  finds out the name of the texture with getTextureName(1),
		  appends "^crack0" to it and gets a new texture id with
		  getTextureId("stone.png^mineral_coal.png^crack0").

	*/

	/*
		Gets a texture id from cache or
		- if main thread, generates the texture, adds to cache and returns id.
		- if other thread, adds to request queue and waits for main thread.

		The id 0 points to a NULL texture. It is returned in case of error.
	*/
	u32 getTextureId(const std::string &name);

	// Finds out the name of a cached texture.
	std::string getTextureName(u32 id);

	/*
		If texture specified by the name pointed by the id doesn't
		exist, create it, then return the cached texture.

		Can be called from any thread. If called from some other thread
		and not found in cache, the call is queued to the main thread
		for processing.
	*/
	video::ITexture* getTexture(u32 id);

	video::ITexture* getTexture(const std::string &name, u32 *id = NULL);

	/*
		Get a texture specifically intended for mesh
		application, i.e. not HUD, compositing, or other 2D
		use.  This texture may be a different size and may
		have had additional filters applied.
	*/
	video::ITexture* getTextureForMesh(const std::string &name, u32 *id);

	virtual Palette* getPalette(const std::string &name);

	// Returns a pointer to the irrlicht device
	virtual IrrlichtDevice* getDevice()
	{
		return m_device;
	}

	bool isKnownSourceImage(const std::string &name)
	{
		bool is_known = false;
		bool cache_found = m_source_image_existence.get(name, &is_known);
		if (cache_found)
			return is_known;
		// Not found in cache; find out if a local file exists
		is_known = (getTexturePath(name) != "");
		m_source_image_existence.set(name, is_known);
		return is_known;
	}

	// Processes queued texture requests from other threads.
	// Shall be called from the main thread.
	void processQueue();

	// Insert an image into the cache without touching the filesystem.
	// Shall be called from the main thread.
	void insertSourceImage(const std::string &name, video::IImage *img);

	// Rebuild images and textures from the current set of source images
	// Shall be called from the main thread.
	void rebuildImagesAndTextures();

	// Render a mesh to a texture.
	// Returns NULL if render-to-texture failed.
	// Shall be called from the main thread.
	video::ITexture* generateTextureFromMesh(
			const TextureFromMeshParams &params);

	video::ITexture* getNormalTexture(const std::string &name);
	video::SColor getTextureAverageColor(const std::string &name);
	video::ITexture *getShaderFlagsTexture(bool normamap_present);

private:

	// The id of the thread that is allowed to use irrlicht directly
	threadid_t m_main_thread;
	// The irrlicht device
	IrrlichtDevice *m_device;

	// Cache of source images
	// This should be only accessed from the main thread
	SourceImageCache m_sourcecache;

	// Generate a texture
	u32 generateTexture(const std::string &name);

	// Generate image based on a string like "stone.png" or "[crack:1:0".
	// if baseimg is NULL, it is created. Otherwise stuff is made on it.
	bool generateImagePart(std::string part_of_name, video::IImage *& baseimg);

	/*! Generates an image from a full string like
	 * "stone.png^mineral_coal.png^[crack:1:0".
	 * Shall be called from the main thread.
	 * The returned Image should be dropped.
	 */
	video::IImage* generateImage(const std::string &name);

	// Thread-safe cache of what source images are known (true = known)
	MutexedMap<std::string, bool> m_source_image_existence;

	// A texture id is index in this array.
	// The first position contains a NULL texture.
	std::vector<TextureInfo> m_textureinfo_cache;
	// Maps a texture name to an index in the former.
	std::map<std::string, u32> m_name_to_id;
	// The two former containers are behind this mutex
	Mutex m_textureinfo_cache_mutex;

	// Queued texture fetches (to be processed by the main thread)
	RequestQueue<std::string, u32, u8, u8> m_get_texture_queue;

	// Textures that have been overwritten with other ones
	// but can't be deleted because the ITexture* might still be used
	std::vector<video::ITexture*> m_texture_trash;

	// Maps image file names to loaded palettes.
	UNORDERED_MAP<std::string, Palette> m_palettes;

	// Cached settings needed for making textures from meshes
	bool m_setting_trilinear_filter;
	bool m_setting_bilinear_filter;
	bool m_setting_anisotropic_filter;
};

IWritableTextureSource* createTextureSource(IrrlichtDevice *device)
{
	return new TextureSource(device);
}

TextureSource::TextureSource(IrrlichtDevice *device):
		m_device(device)
{
	assert(m_device); // Pre-condition

	m_main_thread = thr_get_current_thread_id();

	// Add a NULL TextureInfo as the first index, named ""
	m_textureinfo_cache.push_back(TextureInfo(""));
	m_name_to_id[""] = 0;

	// Cache some settings
	// Note: Since this is only done once, the game must be restarted
	// for these settings to take effect
	m_setting_trilinear_filter = g_settings->getBool("trilinear_filter");
	m_setting_bilinear_filter = g_settings->getBool("bilinear_filter");
	m_setting_anisotropic_filter = g_settings->getBool("anisotropic_filter");
}

TextureSource::~TextureSource()
{
	video::IVideoDriver* driver = m_device->getVideoDriver();

	unsigned int textures_before = driver->getTextureCount();

	for (std::vector<TextureInfo>::iterator iter =
			m_textureinfo_cache.begin();
			iter != m_textureinfo_cache.end(); ++iter)
	{
		//cleanup texture
		if (iter->texture)
			driver->removeTexture(iter->texture);
	}
	m_textureinfo_cache.clear();

	for (std::vector<video::ITexture*>::iterator iter =
			m_texture_trash.begin(); iter != m_texture_trash.end();
			++iter) {
		video::ITexture *t = *iter;

		//cleanup trashed texture
		driver->removeTexture(t);
	}

	infostream << "~TextureSource() "<< textures_before << "/"
			<< driver->getTextureCount() << std::endl;
}

u32 TextureSource::getTextureId(const std::string &name)
{
	//infostream<<"getTextureId(): \""<<name<<"\""<<std::endl;

	{
		/*
			See if texture already exists
		*/
		MutexAutoLock lock(m_textureinfo_cache_mutex);
		std::map<std::string, u32>::iterator n;
		n = m_name_to_id.find(name);
		if (n != m_name_to_id.end())
		{
			return n->second;
		}
	}

	/*
		Get texture
	*/
	if (thr_is_current_thread(m_main_thread))
	{
		return generateTexture(name);
	}
	else
	{
		infostream<<"getTextureId(): Queued: name=\""<<name<<"\""<<std::endl;

		// We're gonna ask the result to be put into here
		static ResultQueue<std::string, u32, u8, u8> result_queue;

		// Throw a request in
		m_get_texture_queue.add(name, 0, 0, &result_queue);

		/*infostream<<"Waiting for texture from main thread, name=\""
				<<name<<"\""<<std::endl;*/

		try
		{
			while(true) {
				// Wait result for a second
				GetResult<std::string, u32, u8, u8>
					result = result_queue.pop_front(1000);

				if (result.key == name) {
					return result.item;
				}
			}
		}
		catch(ItemNotFoundException &e)
		{
			errorstream<<"Waiting for texture " << name << " timed out."<<std::endl;
			return 0;
		}
	}

	infostream<<"getTextureId(): Failed"<<std::endl;

	return 0;
}

// Draw an image on top of an another one, using the alpha channel of the
// source image
static void blit_with_alpha(video::IImage *src, video::IImage *dst,
		v2s32 src_pos, v2s32 dst_pos, v2u32 size);

// Like blit_with_alpha, but only modifies destination pixels that
// are fully opaque
static void blit_with_alpha_overlay(video::IImage *src, video::IImage *dst,
		v2s32 src_pos, v2s32 dst_pos, v2u32 size);

// Apply a color to an image.  Uses an int (0-255) to calculate the ratio.
// If the ratio is 255 or -1 and keep_alpha is true, then it multiples the
// color alpha with the destination alpha.
// Otherwise, any pixels that are not fully transparent get the color alpha.
static void apply_colorize(video::IImage *dst, v2u32 dst_pos, v2u32 size,
		const video::SColor &color, int ratio, bool keep_alpha);

// paint a texture using the given color
static void apply_multiplication(video::IImage *dst, v2u32 dst_pos, v2u32 size,
		const video::SColor &color);

// Apply a mask to an image
static void apply_mask(video::IImage *mask, video::IImage *dst,
		v2s32 mask_pos, v2s32 dst_pos, v2u32 size);

// Draw or overlay a crack
static void draw_crack(video::IImage *crack, video::IImage *dst,
		bool use_overlay, s32 frame_count, s32 progression,
		video::IVideoDriver *driver);

// Brighten image
void brighten(video::IImage *image);
// Parse a transform name
u32 parseImageTransform(const std::string& s);
// Apply transform to image dimension
core::dimension2d<u32> imageTransformDimension(u32 transform, core::dimension2d<u32> dim);
// Apply transform to image data
void imageTransform(u32 transform, video::IImage *src, video::IImage *dst);

/*
	This method generates all the textures
*/
u32 TextureSource::generateTexture(const std::string &name)
{
	//infostream << "generateTexture(): name=\"" << name << "\"" << std::endl;

	// Empty name means texture 0
	if (name == "") {
		infostream<<"generateTexture(): name is empty"<<std::endl;
		return 0;
	}

	{
		/*
			See if texture already exists
		*/
		MutexAutoLock lock(m_textureinfo_cache_mutex);
		std::map<std::string, u32>::iterator n;
		n = m_name_to_id.find(name);
		if (n != m_name_to_id.end()) {
			return n->second;
		}
	}

	/*
		Calling only allowed from main thread
	*/
	if (!thr_is_current_thread(m_main_thread)) {
		errorstream<<"TextureSource::generateTexture() "
				"called not from main thread"<<std::endl;
		return 0;
	}

	video::IVideoDriver *driver = m_device->getVideoDriver();
	sanity_check(driver);

	video::IImage *img = generateImage(name);

	video::ITexture *tex = NULL;

	if (img != NULL) {
#ifdef __ANDROID__
		img = Align2Npot2(img, driver);
#endif
		// Create texture from resulting image
		tex = driver->addTexture(name.c_str(), img);
		guiScalingCache(io::path(name.c_str()), driver, img);
		img->drop();
	}

	/*
		Add texture to caches (add NULL textures too)
	*/

	MutexAutoLock lock(m_textureinfo_cache_mutex);

	u32 id = m_textureinfo_cache.size();
	TextureInfo ti(name, tex);
	m_textureinfo_cache.push_back(ti);
	m_name_to_id[name] = id;

	return id;
}

std::string TextureSource::getTextureName(u32 id)
{
	MutexAutoLock lock(m_textureinfo_cache_mutex);

	if (id >= m_textureinfo_cache.size())
	{
		errorstream<<"TextureSource::getTextureName(): id="<<id
				<<" >= m_textureinfo_cache.size()="
				<<m_textureinfo_cache.size()<<std::endl;
		return "";
	}

	return m_textureinfo_cache[id].name;
}

video::ITexture* TextureSource::getTexture(u32 id)
{
	MutexAutoLock lock(m_textureinfo_cache_mutex);

	if (id >= m_textureinfo_cache.size())
		return NULL;

	return m_textureinfo_cache[id].texture;
}

video::ITexture* TextureSource::getTexture(const std::string &name, u32 *id)
{
	u32 actual_id = getTextureId(name);
	if (id){
		*id = actual_id;
	}
	return getTexture(actual_id);
}

video::ITexture* TextureSource::getTextureForMesh(const std::string &name, u32 *id)
{
	return getTexture(name + "^[applyfiltersformesh", id);
}

Palette* TextureSource::getPalette(const std::string &name)
{
	// Only the main thread may load images
	sanity_check(thr_is_current_thread(m_main_thread));

	if (name == "")
		return NULL;

	UNORDERED_MAP<std::string, Palette>::iterator it = m_palettes.find(name);
	if (it == m_palettes.end()) {
		// Create palette
		video::IImage *img = generateImage(name);
		if (!img) {
			warningstream << "TextureSource::getPalette(): palette \"" << name
				<< "\" could not be loaded." << std::endl;
			return NULL;
		}
		Palette new_palette;
		u32 w = img->getDimension().Width;
		u32 h = img->getDimension().Height;
		// Real area of the image
		u32 area = h * w;
		if (area == 0)
			return NULL;
		if (area > 256) {
			warningstream << "TextureSource::getPalette(): the specified"
				<< " palette image \"" << name << "\" is larger than 256"
				<< " pixels, using the first 256." << std::endl;
			area = 256;
		} else if (256 % area != 0)
			warningstream << "TextureSource::getPalette(): the "
				<< "specified palette image \"" << name << "\" does not "
				<< "contain power of two pixels." << std::endl;
		// We stretch the palette so it will fit 256 values
		// This many param2 values will have the same color
		u32 step = 256 / area;
		// For each pixel in the image
		for (u32 i = 0; i < area; i++) {
			video::SColor c = img->getPixel(i % w, i / w);
			// Fill in palette with 'step' colors
			for (u32 j = 0; j < step; j++)
				new_palette.push_back(c);
		}
		img->drop();
		// Fill in remaining elements
		while (new_palette.size() < 256)
			new_palette.push_back(video::SColor(0xFFFFFFFF));
		m_palettes[name] = new_palette;
		it = m_palettes.find(name);
	}
	if (it != m_palettes.end())
		return &((*it).second);
	return NULL;
}

void TextureSource::processQueue()
{
	/*
		Fetch textures
	*/
	//NOTE this is only thread safe for ONE consumer thread!
	if (!m_get_texture_queue.empty())
	{
		GetRequest<std::string, u32, u8, u8>
				request = m_get_texture_queue.pop();

		/*infostream<<"TextureSource::processQueue(): "
				<<"got texture request with "
				<<"name=\""<<request.key<<"\""
				<<std::endl;*/

		m_get_texture_queue.pushResult(request, generateTexture(request.key));
	}
}

void TextureSource::insertSourceImage(const std::string &name, video::IImage *img)
{
	//infostream<<"TextureSource::insertSourceImage(): name="<<name<<std::endl;

	sanity_check(thr_is_current_thread(m_main_thread));

	m_sourcecache.insert(name, img, true, m_device->getVideoDriver());
	m_source_image_existence.set(name, true);
}

void TextureSource::rebuildImagesAndTextures()
{
	MutexAutoLock lock(m_textureinfo_cache_mutex);

	video::IVideoDriver* driver = m_device->getVideoDriver();
	sanity_check(driver);

	// Recreate textures
	for (u32 i=0; i<m_textureinfo_cache.size(); i++){
		TextureInfo *ti = &m_textureinfo_cache[i];
		video::IImage *img = generateImage(ti->name);
#ifdef __ANDROID__
		img = Align2Npot2(img, driver);
#endif
		// Create texture from resulting image
		video::ITexture *t = NULL;
		if (img) {
			t = driver->addTexture(ti->name.c_str(), img);
			guiScalingCache(io::path(ti->name.c_str()), driver, img);
			img->drop();
		}
		video::ITexture *t_old = ti->texture;
		// Replace texture
		ti->texture = t;

		if (t_old)
			m_texture_trash.push_back(t_old);
	}
}

video::ITexture* TextureSource::generateTextureFromMesh(
		const TextureFromMeshParams &params)
{
	video::IVideoDriver *driver = m_device->getVideoDriver();
	sanity_check(driver);

#ifdef __ANDROID__
	const GLubyte* renderstr = glGetString(GL_RENDERER);
	std::string renderer((char*) renderstr);

	// use no render to texture hack
	if (
		(renderer.find("Adreno") != std::string::npos) ||
		(renderer.find("Mali") != std::string::npos) ||
		(renderer.find("Immersion") != std::string::npos) ||
		(renderer.find("Tegra") != std::string::npos) ||
		g_settings->getBool("inventory_image_hack")
		) {
		// Get a scene manager
		scene::ISceneManager *smgr_main = m_device->getSceneManager();
		sanity_check(smgr_main);
		scene::ISceneManager *smgr = smgr_main->createNewSceneManager();
		sanity_check(smgr);

		const float scaling = 0.2;

		scene::IMeshSceneNode* meshnode =
				smgr->addMeshSceneNode(params.mesh, NULL,
						-1, v3f(0,0,0), v3f(0,0,0),
						v3f(1.0 * scaling,1.0 * scaling,1.0 * scaling), true);
		meshnode->setMaterialFlag(video::EMF_LIGHTING, true);
		meshnode->setMaterialFlag(video::EMF_ANTI_ALIASING, true);
		meshnode->setMaterialFlag(video::EMF_TRILINEAR_FILTER, m_setting_trilinear_filter);
		meshnode->setMaterialFlag(video::EMF_BILINEAR_FILTER, m_setting_bilinear_filter);
		meshnode->setMaterialFlag(video::EMF_ANISOTROPIC_FILTER, m_setting_anisotropic_filter);

		scene::ICameraSceneNode* camera = smgr->addCameraSceneNode(0,
				params.camera_position, params.camera_lookat);
		// second parameter of setProjectionMatrix (isOrthogonal) is ignored
		camera->setProjectionMatrix(params.camera_projection_matrix, false);

		smgr->setAmbientLight(params.ambient_light);
		smgr->addLightSceneNode(0,
				params.light_position,
				params.light_color,
				params.light_radius*scaling);

		core::dimension2d<u32> screen = driver->getScreenSize();

		// Render scene
		driver->beginScene(true, true, video::SColor(0,0,0,0));
		driver->clearZBuffer();
		smgr->drawAll();

		core::dimension2d<u32> partsize(screen.Width * scaling,screen.Height * scaling);

		irr::video::IImage* rawImage =
				driver->createImage(irr::video::ECF_A8R8G8B8, partsize);

		u8* pixels = static_cast<u8*>(rawImage->lock());
		if (!pixels)
		{
			rawImage->drop();
			return NULL;
		}

		core::rect<s32> source(
				screen.Width /2 - (screen.Width  * (scaling / 2)),
				screen.Height/2 - (screen.Height * (scaling / 2)),
				screen.Width /2 + (screen.Width  * (scaling / 2)),
				screen.Height/2 + (screen.Height * (scaling / 2))
			);

		glReadPixels(source.UpperLeftCorner.X, source.UpperLeftCorner.Y,
				partsize.Width, partsize.Height, GL_RGBA,
				GL_UNSIGNED_BYTE, pixels);

		driver->endScene();

		// Drop scene manager
		smgr->drop();

		unsigned int pixelcount = partsize.Width*partsize.Height;

		u8* runptr = pixels;
		for (unsigned int i=0; i < pixelcount; i++) {

			u8 B = *runptr;
			u8 G = *(runptr+1);
			u8 R = *(runptr+2);
			u8 A = *(runptr+3);

			//BGRA -> RGBA
			*runptr = R;
			runptr ++;
			*runptr = G;
			runptr ++;
			*runptr = B;
			runptr ++;
			*runptr = A;
			runptr ++;
		}

		video::IImage* inventory_image =
				driver->createImage(irr::video::ECF_A8R8G8B8, params.dim);

		rawImage->copyToScaling(inventory_image);
		rawImage->drop();

		guiScalingCache(io::path(params.rtt_texture_name.c_str()), driver, inventory_image);

		video::ITexture *rtt = driver->addTexture(params.rtt_texture_name.c_str(), inventory_image);
		inventory_image->drop();

		if (rtt == NULL) {
			errorstream << "TextureSource::generateTextureFromMesh(): failed to recreate texture from image: " << params.rtt_texture_name << std::endl;
			return NULL;
		}

		driver->makeColorKeyTexture(rtt, v2s32(0,0));

		if (params.delete_texture_on_shutdown)
			m_texture_trash.push_back(rtt);

		return rtt;
	}
#endif

	if (driver->queryFeature(video::EVDF_RENDER_TO_TARGET) == false)
	{
		static bool warned = false;
		if (!warned)
		{
			errorstream<<"TextureSource::generateTextureFromMesh(): "
				<<"EVDF_RENDER_TO_TARGET not supported."<<std::endl;
			warned = true;
		}
		return NULL;
	}

	// Create render target texture
	video::ITexture *rtt = driver->addRenderTargetTexture(
			params.dim, params.rtt_texture_name.c_str(),
			video::ECF_A8R8G8B8);
	if (rtt == NULL)
	{
		errorstream<<"TextureSource::generateTextureFromMesh(): "
			<<"addRenderTargetTexture returned NULL."<<std::endl;
		return NULL;
	}

	// Set render target
	if (!driver->setRenderTarget(rtt, false, true, video::SColor(0,0,0,0))) {
		driver->removeTexture(rtt);
		errorstream<<"TextureSource::generateTextureFromMesh(): "
			<<"failed to set render target"<<std::endl;
		return NULL;
	}

	// Get a scene manager
	scene::ISceneManager *smgr_main = m_device->getSceneManager();
	assert(smgr_main);
	scene::ISceneManager *smgr = smgr_main->createNewSceneManager();
	assert(smgr);

	scene::IMeshSceneNode* meshnode =
			smgr->addMeshSceneNode(params.mesh, NULL,
					-1, v3f(0,0,0), v3f(0,0,0), v3f(1,1,1), true);
	meshnode->setMaterialFlag(video::EMF_LIGHTING, true);
	meshnode->setMaterialFlag(video::EMF_ANTI_ALIASING, true);
	meshnode->setMaterialFlag(video::EMF_TRILINEAR_FILTER, m_setting_trilinear_filter);
	meshnode->setMaterialFlag(video::EMF_BILINEAR_FILTER, m_setting_bilinear_filter);
	meshnode->setMaterialFlag(video::EMF_ANISOTROPIC_FILTER, m_setting_anisotropic_filter);

	scene::ICameraSceneNode* camera = smgr->addCameraSceneNode(0,
			params.camera_position, params.camera_lookat);
	// second parameter of setProjectionMatrix (isOrthogonal) is ignored
	camera->setProjectionMatrix(params.camera_projection_matrix, false);

	smgr->setAmbientLight(params.ambient_light);
	smgr->addLightSceneNode(0,
			params.light_position,
			params.light_color,
			params.light_radius);

	// Render scene
	driver->beginScene(true, true, video::SColor(0,0,0,0));
	smgr->drawAll();
	driver->endScene();

	// Drop scene manager
	smgr->drop();

	// Unset render target
	driver->setRenderTarget(0, false, true, video::SColor(0,0,0,0));

	if (params.delete_texture_on_shutdown)
		m_texture_trash.push_back(rtt);

	return rtt;
}

video::IImage* TextureSource::generateImage(const std::string &name)
{
	// Get the base image

	const char separator = '^';
	const char escape = '\\';
	const char paren_open = '(';
	const char paren_close = ')';

	// Find last separator in the name
	s32 last_separator_pos = -1;
	u8 paren_bal = 0;
	for (s32 i = name.size() - 1; i >= 0; i--) {
		if (i > 0 && name[i-1] == escape)
			continue;
		switch (name[i]) {
		case separator:
			if (paren_bal == 0) {
				last_separator_pos = i;
				i = -1; // break out of loop
			}
			break;
		case paren_open:
			if (paren_bal == 0) {
				errorstream << "generateImage(): unbalanced parentheses"
						<< "(extranous '(') while generating texture \""
						<< name << "\"" << std::endl;
				return NULL;
			}
			paren_bal--;
			break;
		case paren_close:
			paren_bal++;
			break;
		default:
			break;
		}
	}
	if (paren_bal > 0) {
		errorstream << "generateImage(): unbalanced parentheses"
				<< "(missing matching '(') while generating texture \""
				<< name << "\"" << std::endl;
		return NULL;
	}


	video::IImage *baseimg = NULL;

	/*
		If separator was found, make the base image
		using a recursive call.
	*/
	if (last_separator_pos != -1) {
		baseimg = generateImage(name.substr(0, last_separator_pos));
	}


	video::IVideoDriver* driver = m_device->getVideoDriver();
	sanity_check(driver);

	/*
		Parse out the last part of the name of the image and act
		according to it
	*/

	std::string last_part_of_name = name.substr(last_separator_pos + 1);

	/*
		If this name is enclosed in parentheses, generate it
		and blit it onto the base image
	*/
	if (last_part_of_name[0] == paren_open
			&& last_part_of_name[last_part_of_name.size() - 1] == paren_close) {
		std::string name2 = last_part_of_name.substr(1,
				last_part_of_name.size() - 2);
		video::IImage *tmp = generateImage(name2);
		if (!tmp) {
			errorstream << "generateImage(): "
				"Failed to generate \"" << name2 << "\""
				<< std::endl;
			return NULL;
		}
		core::dimension2d<u32> dim = tmp->getDimension();
		if (baseimg) {
			blit_with_alpha(tmp, baseimg, v2s32(0, 0), v2s32(0, 0), dim);
			tmp->drop();
		} else {
			baseimg = tmp;
		}
	} else if (!generateImagePart(last_part_of_name, baseimg)) {
		// Generate image according to part of name
		errorstream << "generateImage(): "
				"Failed to generate \"" << last_part_of_name << "\""
				<< std::endl;
	}

	// If no resulting image, print a warning
	if (baseimg == NULL) {
		errorstream << "generateImage(): baseimg is NULL (attempted to"
				" create texture \"" << name << "\")" << std::endl;
	}

	return baseimg;
}

#ifdef __ANDROID__
#include <GLES/gl.h>
/**
 * Check and align image to npot2 if required by hardware
 * @param image image to check for npot2 alignment
 * @param driver driver to use for image operations
 * @return image or copy of image aligned to npot2
 */

inline u16 get_GL_major_version()
{
	const GLubyte *gl_version = glGetString(GL_VERSION);
	return (u16) (gl_version[0] - '0');
}

video::IImage * Align2Npot2(video::IImage * image,
		video::IVideoDriver* driver)
{
	if (image == NULL) {
		return image;
	}

	core::dimension2d<u32> dim = image->getDimension();

	std::string extensions = (char*) glGetString(GL_EXTENSIONS);

	// Only GLES2 is trusted to correctly report npot support
	if (get_GL_major_version() > 1 &&
			extensions.find("GL_OES_texture_npot") != std::string::npos) {
		return image;
	}

	unsigned int height = npot2(dim.Height);
	unsigned int width  = npot2(dim.Width);

	if ((dim.Height == height) &&
			(dim.Width == width)) {
		return image;
	}

	if (dim.Height > height) {
		height *= 2;
	}

	if (dim.Width > width) {
		width *= 2;
	}

	video::IImage *targetimage =
			driver->createImage(video::ECF_A8R8G8B8,
					core::dimension2d<u32>(width, height));

	if (targetimage != NULL) {
		image->copyToScaling(targetimage);
	}
	image->drop();
	return targetimage;
}

#endif

static std::string unescape_string(const std::string &str, const char esc = '\\')
{
	std::string out;
	size_t pos = 0, cpos;
	out.reserve(str.size());
	while (1) {
		cpos = str.find_first_of(esc, pos);
		if (cpos == std::string::npos) {
			out += str.substr(pos);
			break;
		}
		out += str.substr(pos, cpos - pos) + str[cpos + 1];
		pos = cpos + 2;
	}
	return out;
}

bool TextureSource::generateImagePart(std::string part_of_name,
		video::IImage *& baseimg)
{
	const char escape = '\\'; // same as in generateImage()
	video::IVideoDriver* driver = m_device->getVideoDriver();
	sanity_check(driver);

	// Stuff starting with [ are special commands
	if (part_of_name.size() == 0 || part_of_name[0] != '[')
	{
		video::IImage *image = m_sourcecache.getOrLoad(part_of_name, m_device);
#ifdef __ANDROID__
		image = Align2Npot2(image, driver);
#endif
		if (image == NULL) {
			if (part_of_name != "") {

				// Do not create normalmap dummies
				if (part_of_name.find("_normal.png") != std::string::npos) {
					warningstream << "generateImage(): Could not load normal map \""
						<< part_of_name << "\"" << std::endl;
					return true;
				}

				errorstream << "generateImage(): Could not load image \""
					<< part_of_name << "\" while building texture; "
					"Creating a dummy image" << std::endl;
			}

			// Just create a dummy image
			//core::dimension2d<u32> dim(2,2);
			core::dimension2d<u32> dim(1,1);
			image = driver->createImage(video::ECF_A8R8G8B8, dim);
			sanity_check(image != NULL);
			/*image->setPixel(0,0, video::SColor(255,255,0,0));
			image->setPixel(1,0, video::SColor(255,0,255,0));
			image->setPixel(0,1, video::SColor(255,0,0,255));
			image->setPixel(1,1, video::SColor(255,255,0,255));*/
			image->setPixel(0,0, video::SColor(255,myrand()%256,
					myrand()%256,myrand()%256));
			/*image->setPixel(1,0, video::SColor(255,myrand()%256,
					myrand()%256,myrand()%256));
			image->setPixel(0,1, video::SColor(255,myrand()%256,
					myrand()%256,myrand()%256));
			image->setPixel(1,1, video::SColor(255,myrand()%256,
					myrand()%256,myrand()%256));*/
		}

		// If base image is NULL, load as base.
		if (baseimg == NULL)
		{
			//infostream<<"Setting "<<part_of_name<<" as base"<<std::endl;
			/*
				Copy it this way to get an alpha channel.
				Otherwise images with alpha cannot be blitted on
				images that don't have alpha in the original file.
			*/
			core::dimension2d<u32> dim = image->getDimension();
			baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);
			image->copyTo(baseimg);
		}
		// Else blit on base.
		else
		{
			//infostream<<"Blitting "<<part_of_name<<" on base"<<std::endl;
			// Size of the copied area
			core::dimension2d<u32> dim = image->getDimension();
			//core::dimension2d<u32> dim(16,16);
			// Position to copy the blitted to in the base image
			core::position2d<s32> pos_to(0,0);
			// Position to copy the blitted from in the blitted image
			core::position2d<s32> pos_from(0,0);
			// Blit
			/*image->copyToWithAlpha(baseimg, pos_to,
					core::rect<s32>(pos_from, dim),
					video::SColor(255,255,255,255),
					NULL);*/

			core::dimension2d<u32> dim_dst = baseimg->getDimension();
			if (dim == dim_dst) {
				blit_with_alpha(image, baseimg, pos_from, pos_to, dim);
			} else if (dim.Width * dim.Height < dim_dst.Width * dim_dst.Height) {
				// Upscale overlying image
				video::IImage* scaled_image = m_device->getVideoDriver()->
					createImage(video::ECF_A8R8G8B8, dim_dst);
				image->copyToScaling(scaled_image);

				blit_with_alpha(scaled_image, baseimg, pos_from, pos_to, dim_dst);
				scaled_image->drop();
			} else {
				// Upscale base image
				video::IImage* scaled_base = m_device->getVideoDriver()->
					createImage(video::ECF_A8R8G8B8, dim);
				baseimg->copyToScaling(scaled_base);
				baseimg->drop();
				baseimg = scaled_base;

				blit_with_alpha(image, baseimg, pos_from, pos_to, dim);
			}
		}
		//cleanup
		image->drop();
	}
	else
	{
		// A special texture modification

		/*infostream<<"generateImage(): generating special "
				<<"modification \""<<part_of_name<<"\""
				<<std::endl;*/

		/*
			[crack:N:P
			[cracko:N:P
			Adds a cracking texture
			N = animation frame count, P = crack progression
		*/
		if (str_starts_with(part_of_name, "[crack"))
		{
			if (baseimg == NULL) {
				errorstream<<"generateImagePart(): baseimg == NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			// Crack image number and overlay option
			bool use_overlay = (part_of_name[6] == 'o');
			Strfnd sf(part_of_name);
			sf.next(":");
			s32 frame_count = stoi(sf.next(":"));
			s32 progression = stoi(sf.next(":"));

			if (progression >= 0) {
				/*
					Load crack image.

					It is an image with a number of cracking stages
					horizontally tiled.
				*/
				video::IImage *img_crack = m_sourcecache.getOrLoad(
					"crack_anylength.png", m_device);

				if (img_crack) {
					draw_crack(img_crack, baseimg,
						use_overlay, frame_count,
						progression, driver);
					img_crack->drop();
				}
			}
		}
		/*
			[combine:WxH:X,Y=filename:X,Y=filename2
			Creates a bigger texture from any amount of smaller ones
		*/
		else if (str_starts_with(part_of_name, "[combine"))
		{
			Strfnd sf(part_of_name);
			sf.next(":");
			u32 w0 = stoi(sf.next("x"));
			u32 h0 = stoi(sf.next(":"));
			core::dimension2d<u32> dim(w0,h0);
			if (baseimg == NULL) {
				baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);
				baseimg->fill(video::SColor(0,0,0,0));
			}
			while (sf.at_end() == false) {
				u32 x = stoi(sf.next(","));
				u32 y = stoi(sf.next("="));
				std::string filename = unescape_string(sf.next_esc(":", escape), escape);
				infostream<<"Adding \""<<filename
						<<"\" to combined ("<<x<<","<<y<<")"
						<<std::endl;
				video::IImage *img = generateImage(filename);
				if (img) {
					core::dimension2d<u32> dim = img->getDimension();
					infostream<<"Size "<<dim.Width
							<<"x"<<dim.Height<<std::endl;
					core::position2d<s32> pos_base(x, y);
					video::IImage *img2 =
							driver->createImage(video::ECF_A8R8G8B8, dim);
					img->copyTo(img2);
					img->drop();
					/*img2->copyToWithAlpha(baseimg, pos_base,
							core::rect<s32>(v2s32(0,0), dim),
							video::SColor(255,255,255,255),
							NULL);*/
					blit_with_alpha(img2, baseimg, v2s32(0,0), pos_base, dim);
					img2->drop();
				} else {
					errorstream << "generateImagePart(): Failed to load image \""
						<< filename << "\" for [combine" << std::endl;
				}
			}
		}
		/*
			[brighten
		*/
		else if (str_starts_with(part_of_name, "[brighten"))
		{
			if (baseimg == NULL) {
				errorstream<<"generateImagePart(): baseimg==NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			brighten(baseimg);
		}
		/*
			[noalpha
			Make image completely opaque.
			Used for the leaves texture when in old leaves mode, so
			that the transparent parts don't look completely black
			when simple alpha channel is used for rendering.
		*/
		else if (str_starts_with(part_of_name, "[noalpha"))
		{
			if (baseimg == NULL){
				errorstream<<"generateImagePart(): baseimg==NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			core::dimension2d<u32> dim = baseimg->getDimension();

			// Set alpha to full
			for (u32 y=0; y<dim.Height; y++)
			for (u32 x=0; x<dim.Width; x++)
			{
				video::SColor c = baseimg->getPixel(x,y);
				c.setAlpha(255);
				baseimg->setPixel(x,y,c);
			}
		}
		/*
			[makealpha:R,G,B
			Convert one color to transparent.
		*/
		else if (str_starts_with(part_of_name, "[makealpha:"))
		{
			if (baseimg == NULL) {
				errorstream<<"generateImagePart(): baseimg == NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			Strfnd sf(part_of_name.substr(11));
			u32 r1 = stoi(sf.next(","));
			u32 g1 = stoi(sf.next(","));
			u32 b1 = stoi(sf.next(""));

			core::dimension2d<u32> dim = baseimg->getDimension();

			/*video::IImage *oldbaseimg = baseimg;
			baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);
			oldbaseimg->copyTo(baseimg);
			oldbaseimg->drop();*/

			// Set alpha to full
			for (u32 y=0; y<dim.Height; y++)
			for (u32 x=0; x<dim.Width; x++)
			{
				video::SColor c = baseimg->getPixel(x,y);
				u32 r = c.getRed();
				u32 g = c.getGreen();
				u32 b = c.getBlue();
				if (!(r == r1 && g == g1 && b == b1))
					continue;
				c.setAlpha(0);
				baseimg->setPixel(x,y,c);
			}
		}
		/*
			[transformN
			Rotates and/or flips the image.

			N can be a number (between 0 and 7) or a transform name.
			Rotations are counter-clockwise.
			0  I      identity
			1  R90    rotate by 90 degrees
			2  R180   rotate by 180 degrees
			3  R270   rotate by 270 degrees
			4  FX     flip X
			5  FXR90  flip X then rotate by 90 degrees
			6  FY     flip Y
			7  FYR90  flip Y then rotate by 90 degrees

			Note: Transform names can be concatenated to produce
			their product (applies the first then the second).
			The resulting transform will be equivalent to one of the
			eight existing ones, though (see: dihedral group).
		*/
		else if (str_starts_with(part_of_name, "[transform"))
		{
			if (baseimg == NULL) {
				errorstream<<"generateImagePart(): baseimg == NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			u32 transform = parseImageTransform(part_of_name.substr(10));
			core::dimension2d<u32> dim = imageTransformDimension(
					transform, baseimg->getDimension());
			video::IImage *image = driver->createImage(
					baseimg->getColorFormat(), dim);
			sanity_check(image != NULL);
			imageTransform(transform, baseimg, image);
			baseimg->drop();
			baseimg = image;
		}
		/*
			[inventorycube{topimage{leftimage{rightimage
			In every subimage, replace ^ with &.
			Create an "inventory cube".
			NOTE: This should be used only on its own.
			Example (a grass block (not actually used in game):
			"[inventorycube{grass.png{mud.png&grass_side.png{mud.png&grass_side.png"
		*/
		else if (str_starts_with(part_of_name, "[inventorycube"))
		{
			if (baseimg != NULL){
				errorstream<<"generateImagePart(): baseimg != NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			str_replace(part_of_name, '&', '^');
			Strfnd sf(part_of_name);
			sf.next("{");
			std::string imagename_top = sf.next("{");
			std::string imagename_left = sf.next("{");
			std::string imagename_right = sf.next("{");

			// Generate images for the faces of the cube
			video::IImage *img_top = generateImage(imagename_top);
			video::IImage *img_left = generateImage(imagename_left);
			video::IImage *img_right = generateImage(imagename_right);

			if (img_top == NULL || img_left == NULL || img_right == NULL) {
				errorstream << "generateImagePart(): Failed to create textures"
						<< " for inventorycube \"" << part_of_name << "\""
						<< std::endl;
				baseimg = generateImage(imagename_top);
				return true;
			}

#ifdef __ANDROID__
			assert(img_top->getDimension().Height == npot2(img_top->getDimension().Height));
			assert(img_top->getDimension().Width == npot2(img_top->getDimension().Width));

			assert(img_left->getDimension().Height == npot2(img_left->getDimension().Height));
			assert(img_left->getDimension().Width == npot2(img_left->getDimension().Width));

			assert(img_right->getDimension().Height == npot2(img_right->getDimension().Height));
			assert(img_right->getDimension().Width == npot2(img_right->getDimension().Width));
#endif

			// Create textures from images
			video::ITexture *texture_top = driver->addTexture(
					(imagename_top + "__temp__").c_str(), img_top);
			video::ITexture *texture_left = driver->addTexture(
					(imagename_left + "__temp__").c_str(), img_left);
			video::ITexture *texture_right = driver->addTexture(
					(imagename_right + "__temp__").c_str(), img_right);
			FATAL_ERROR_IF(!(texture_top && texture_left && texture_right), "");

			// Drop images
			img_top->drop();
			img_left->drop();
			img_right->drop();

			/*
				Draw a cube mesh into a render target texture
			*/
			scene::IMesh* cube = createCubeMesh(v3f(1, 1, 1));
			setMeshColor(cube, video::SColor(255, 255, 255, 255));
			cube->getMeshBuffer(0)->getMaterial().setTexture(0, texture_top);
			cube->getMeshBuffer(1)->getMaterial().setTexture(0, texture_top);
			cube->getMeshBuffer(2)->getMaterial().setTexture(0, texture_right);
			cube->getMeshBuffer(3)->getMaterial().setTexture(0, texture_right);
			cube->getMeshBuffer(4)->getMaterial().setTexture(0, texture_left);
			cube->getMeshBuffer(5)->getMaterial().setTexture(0, texture_left);

			TextureFromMeshParams params;
			params.mesh = cube;
			params.dim.set(64, 64);
			params.rtt_texture_name = part_of_name + "_RTT";
			// We will delete the rtt texture ourselves
			params.delete_texture_on_shutdown = false;
			params.camera_position.set(0, 1.0, -1.5);
			params.camera_position.rotateXZBy(45);
			params.camera_lookat.set(0, 0, 0);
			// Set orthogonal projection
			params.camera_projection_matrix.buildProjectionMatrixOrthoLH(
					1.65, 1.65, 0, 100);

			params.ambient_light.set(1.0, 0.2, 0.2, 0.2);
			params.light_position.set(10, 100, -50);
			params.light_color.set(1.0, 0.5, 0.5, 0.5);
			params.light_radius = 1000;

			video::ITexture *rtt = generateTextureFromMesh(params);

			// Drop mesh
			cube->drop();

			// Free textures
			driver->removeTexture(texture_top);
			driver->removeTexture(texture_left);
			driver->removeTexture(texture_right);

			if (rtt == NULL) {
				baseimg = generateImage(imagename_top);
				return true;
			}

			// Create image of render target
			video::IImage *image = driver->createImage(rtt, v2s32(0, 0), params.dim);
			FATAL_ERROR_IF(!image, "Could not create image of render target");

			// Cleanup texture
			driver->removeTexture(rtt);

			baseimg = driver->createImage(video::ECF_A8R8G8B8, params.dim);

			if (image) {
				image->copyTo(baseimg);
				image->drop();
			}
		}
		/*
			[lowpart:percent:filename
			Adds the lower part of a texture
		*/
		else if (str_starts_with(part_of_name, "[lowpart:"))
		{
			Strfnd sf(part_of_name);
			sf.next(":");
			u32 percent = stoi(sf.next(":"));
			std::string filename = unescape_string(sf.next_esc(":", escape), escape);

			if (baseimg == NULL)
				baseimg = driver->createImage(video::ECF_A8R8G8B8, v2u32(16,16));
			video::IImage *img = generateImage(filename);
			if (img)
			{
				core::dimension2d<u32> dim = img->getDimension();
				core::position2d<s32> pos_base(0, 0);
				video::IImage *img2 =
						driver->createImage(video::ECF_A8R8G8B8, dim);
				img->copyTo(img2);
				img->drop();
				core::position2d<s32> clippos(0, 0);
				clippos.Y = dim.Height * (100-percent) / 100;
				core::dimension2d<u32> clipdim = dim;
				clipdim.Height = clipdim.Height * percent / 100 + 1;
				core::rect<s32> cliprect(clippos, clipdim);
				img2->copyToWithAlpha(baseimg, pos_base,
						core::rect<s32>(v2s32(0,0), dim),
						video::SColor(255,255,255,255),
						&cliprect);
				img2->drop();
			}
		}
		/*
			[verticalframe:N:I
			Crops a frame of a vertical animation.
			N = frame count, I = frame index
		*/
		else if (str_starts_with(part_of_name, "[verticalframe:"))
		{
			Strfnd sf(part_of_name);
			sf.next(":");
			u32 frame_count = stoi(sf.next(":"));
			u32 frame_index = stoi(sf.next(":"));

			if (baseimg == NULL){
				errorstream<<"generateImagePart(): baseimg != NULL "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			v2u32 frame_size = baseimg->getDimension();
			frame_size.Y /= frame_count;

			video::IImage *img = driver->createImage(video::ECF_A8R8G8B8,
					frame_size);
			if (!img){
				errorstream<<"generateImagePart(): Could not create image "
						<<"for part_of_name=\""<<part_of_name
						<<"\", cancelling."<<std::endl;
				return false;
			}

			// Fill target image with transparency
			img->fill(video::SColor(0,0,0,0));

			core::dimension2d<u32> dim = frame_size;
			core::position2d<s32> pos_dst(0, 0);
			core::position2d<s32> pos_src(0, frame_index * frame_size.Y);
			baseimg->copyToWithAlpha(img, pos_dst,
					core::rect<s32>(pos_src, dim),
					video::SColor(255,255,255,255),
					NULL);
			// Replace baseimg
			baseimg->drop();
			baseimg = img;
		}
		/*
			[mask:filename
			Applies a mask to an image
		*/
		else if (str_starts_with(part_of_name, "[mask:"))
		{
			if (baseimg == NULL) {
				errorstream << "generateImage(): baseimg == NULL "
						<< "for part_of_name=\"" << part_of_name
						<< "\", cancelling." << std::endl;
				return false;
			}
			Strfnd sf(part_of_name);
			sf.next(":");
			std::string filename = unescape_string(sf.next_esc(":", escape), escape);

			video::IImage *img = generateImage(filename);
			if (img) {
				apply_mask(img, baseimg, v2s32(0, 0), v2s32(0, 0),
						img->getDimension());
				img->drop();
			} else {
				errorstream << "generateImage(): Failed to load \""
						<< filename << "\".";
			}
		}
		/*
		[multiply:color
			multiplys a given color to any pixel of an image
			color = color as ColorString
		*/
		else if (str_starts_with(part_of_name, "[multiply:")) {
			Strfnd sf(part_of_name);
			sf.next(":");
			std::string color_str = sf.next(":");

			if (baseimg == NULL) {
				errorstream << "generateImagePart(): baseimg != NULL "
						<< "for part_of_name=\"" << part_of_name
						<< "\", cancelling." << std::endl;
				return false;
			}

			video::SColor color;

			if (!parseColorString(color_str, color, false))
				return false;

			apply_multiplication(baseimg, v2u32(0, 0), baseimg->getDimension(), color);
		}
		/*
			[colorize:color
			Overlays image with given color
			color = color as ColorString
		*/
		else if (str_starts_with(part_of_name, "[colorize:"))
		{
			Strfnd sf(part_of_name);
			sf.next(":");
			std::string color_str = sf.next(":");
			std::string ratio_str = sf.next(":");

			if (baseimg == NULL) {
				errorstream << "generateImagePart(): baseimg != NULL "
						<< "for part_of_name=\"" << part_of_name
						<< "\", cancelling." << std::endl;
				return false;
			}

			video::SColor color;
			int ratio = -1;
			bool keep_alpha = false;

			if (!parseColorString(color_str, color, false))
				return false;

			if (is_number(ratio_str))
				ratio = mystoi(ratio_str, 0, 255);
			else if (ratio_str == "alpha")
				keep_alpha = true;

			apply_colorize(baseimg, v2u32(0, 0), baseimg->getDimension(), color, ratio, keep_alpha);
		}
		/*
			[applyfiltersformesh
			Internal modifier
		*/
		else if (str_starts_with(part_of_name, "[applyfiltersformesh"))
		{
			// Apply the "clean transparent" filter, if configured.
			if (g_settings->getBool("texture_clean_transparent"))
				imageCleanTransparent(baseimg, 127);

			/* Upscale textures to user's requested minimum size.  This is a trick to make
			 * filters look as good on low-res textures as on high-res ones, by making
			 * low-res textures BECOME high-res ones.  This is helpful for worlds that
			 * mix high- and low-res textures, or for mods with least-common-denominator
			 * textures that don't have the resources to offer high-res alternatives.
			 */
			const bool filter = m_setting_trilinear_filter || m_setting_bilinear_filter;
			const s32 scaleto = filter ? g_settings->getS32("texture_min_size") : 1;
			if (scaleto > 1) {
				const core::dimension2d<u32> dim = baseimg->getDimension();

				/* Calculate scaling needed to make the shortest texture dimension
				 * equal to the target minimum.  If e.g. this is a vertical frames
				 * animation, the short dimension will be the real size.
				 */
				if ((dim.Width == 0) || (dim.Height == 0)) {
					errorstream << "generateImagePart(): Illegal 0 dimension "
						<< "for part_of_name=\""<< part_of_name
						<< "\", cancelling." << std::endl;
					return false;
				}
				u32 xscale = scaleto / dim.Width;
				u32 yscale = scaleto / dim.Height;
				u32 scale = (xscale > yscale) ? xscale : yscale;

				// Never downscale; only scale up by 2x or more.
				if (scale > 1) {
					u32 w = scale * dim.Width;
					u32 h = scale * dim.Height;
					const core::dimension2d<u32> newdim = core::dimension2d<u32>(w, h);
					video::IImage *newimg = driver->createImage(
							baseimg->getColorFormat(), newdim);
					baseimg->copyToScaling(newimg);
					baseimg->drop();
					baseimg = newimg;
				}
			}
		}
		/*
			[resize:WxH
			Resizes the base image to the given dimensions
		*/
		else if (str_starts_with(part_of_name, "[resize"))
		{
			if (baseimg == NULL) {
				errorstream << "generateImagePart(): baseimg == NULL "
						<< "for part_of_name=\""<< part_of_name
						<< "\", cancelling." << std::endl;
				return false;
			}

			Strfnd sf(part_of_name);
			sf.next(":");
			u32 width = stoi(sf.next("x"));
			u32 height = stoi(sf.next(""));
			core::dimension2d<u32> dim(width, height);

			video::IImage* image = m_device->getVideoDriver()->
				createImage(video::ECF_A8R8G8B8, dim);
			baseimg->copyToScaling(image);
			baseimg->drop();
			baseimg = image;
		}
		/*
			[opacity:R
			Makes the base image transparent according to the given ratio.
			R must be between 0 and 255.
			0 means totally transparent.
			255 means totally opaque.
		*/
		else if (str_starts_with(part_of_name, "[opacity:")) {
			if (baseimg == NULL) {
				errorstream << "generateImagePart(): baseimg == NULL "
						<< "for part_of_name=\"" << part_of_name
						<< "\", cancelling." << std::endl;
				return false;
			}

			Strfnd sf(part_of_name);
			sf.next(":");

			u32 ratio = mystoi(sf.next(""), 0, 255);

			core::dimension2d<u32> dim = baseimg->getDimension();

			for (u32 y = 0; y < dim.Height; y++)
			for (u32 x = 0; x < dim.Width; x++)
			{
				video::SColor c = baseimg->getPixel(x, y);
				c.setAlpha(floor((c.getAlpha() * ratio) / 255 + 0.5));
				baseimg->setPixel(x, y, c);
			}
		}
		/*
			[invert:mode
			Inverts the given channels of the base image.
			Mode may contain the characters "r", "g", "b", "a".
			Only the channels that are mentioned in the mode string
			will be inverted.
		*/
		else if (str_starts_with(part_of_name, "[invert:")) {
			if (baseimg == NULL) {
				errorstream << "generateImagePart(): baseimg == NULL "
						<< "for part_of_name=\"" << part_of_name
						<< "\", cancelling." << std::endl;
				return false;
			}

			Strfnd sf(part_of_name);
			sf.next(":");

			std::string mode = sf.next("");
			u32 mask = 0;
			if (mode.find("a") != std::string::npos)
				mask |= 0xff000000UL;
			if (mode.find("r") != std::string::npos)
				mask |= 0x00ff0000UL;
			if (mode.find("g") != std::string::npos)
				mask |= 0x0000ff00UL;
			if (mode.find("b") != std::string::npos)
				mask |= 0x000000ffUL;

			core::dimension2d<u32> dim = baseimg->getDimension();

			for (u32 y = 0; y < dim.Height; y++)
			for (u32 x = 0; x < dim.Width; x++)
			{
				video::SColor c = baseimg->getPixel(x, y);
				c.color ^= mask;
				baseimg->setPixel(x, y, c);
			}
		}
		/*
			[sheet:WxH:X,Y
			Retrieves a tile at position X,Y (in tiles)
			from the base image it assumes to be a
			tilesheet with dimensions W,H (in tiles).
		*/
		else if (part_of_name.substr(0,7) == "[sheet:") {
			if (baseimg == NULL) {
				errorstream << "generateImagePart(): baseimg != NULL "
						<< "for part_of_name=\"" << part_of_name
						<< "\", cancelling." << std::endl;
				return false;
			}

			Strfnd sf(part_of_name);
			sf.next(":");
			u32 w0 = stoi(sf.next("x"));
			u32 h0 = stoi(sf.next(":"));
			u32 x0 = stoi(sf.next(","));
			u32 y0 = stoi(sf.next(":"));

			core::dimension2d<u32> img_dim = baseimg->getDimension();
			core::dimension2d<u32> tile_dim(v2u32(img_dim) / v2u32(w0, h0));

			video::IImage *img = driver->createImage(
					video::ECF_A8R8G8B8, tile_dim);
			if (!img) {
				errorstream << "generateImagePart(): Could not create image "
						<< "for part_of_name=\"" << part_of_name
						<< "\", cancelling." << std::endl;
				return false;
			}

			img->fill(video::SColor(0,0,0,0));
			v2u32 vdim(tile_dim);
			core::rect<s32> rect(v2s32(x0 * vdim.X, y0 * vdim.Y), tile_dim);
			baseimg->copyToWithAlpha(img, v2s32(0), rect,
					video::SColor(255,255,255,255), NULL);

			// Replace baseimg
			baseimg->drop();
			baseimg = img;
		}
		else
		{
			errorstream << "generateImagePart(): Invalid "
					" modification: \"" << part_of_name << "\"" << std::endl;
		}
	}

	return true;
}

/*
	Draw an image on top of an another one, using the alpha channel of the
	source image

	This exists because IImage::copyToWithAlpha() doesn't seem to always
	work.
*/
static void blit_with_alpha(video::IImage *src, video::IImage *dst,
		v2s32 src_pos, v2s32 dst_pos, v2u32 size)
{
	for (u32 y0=0; y0<size.Y; y0++)
	for (u32 x0=0; x0<size.X; x0++)
	{
		s32 src_x = src_pos.X + x0;
		s32 src_y = src_pos.Y + y0;
		s32 dst_x = dst_pos.X + x0;
		s32 dst_y = dst_pos.Y + y0;
		video::SColor src_c = src->getPixel(src_x, src_y);
		video::SColor dst_c = dst->getPixel(dst_x, dst_y);
		dst_c = src_c.getInterpolated(dst_c, (float)src_c.getAlpha()/255.0f);
		dst->setPixel(dst_x, dst_y, dst_c);
	}
}

/*
	Draw an image on top of an another one, using the alpha channel of the
	source image; only modify fully opaque pixels in destinaion
*/
static void blit_with_alpha_overlay(video::IImage *src, video::IImage *dst,
		v2s32 src_pos, v2s32 dst_pos, v2u32 size)
{
	for (u32 y0=0; y0<size.Y; y0++)
	for (u32 x0=0; x0<size.X; x0++)
	{
		s32 src_x = src_pos.X + x0;
		s32 src_y = src_pos.Y + y0;
		s32 dst_x = dst_pos.X + x0;
		s32 dst_y = dst_pos.Y + y0;
		video::SColor src_c = src->getPixel(src_x, src_y);
		video::SColor dst_c = dst->getPixel(dst_x, dst_y);
		if (dst_c.getAlpha() == 255 && src_c.getAlpha() != 0)
		{
			dst_c = src_c.getInterpolated(dst_c, (float)src_c.getAlpha()/255.0f);
			dst->setPixel(dst_x, dst_y, dst_c);
		}
	}
}

// This function has been disabled because it is currently unused.
// Feel free to re-enable if you find it handy.
#if 0
/*
	Draw an image on top of an another one, using the specified ratio
	modify all partially-opaque pixels in the destination.
*/
static void blit_with_interpolate_overlay(video::IImage *src, video::IImage *dst,
		v2s32 src_pos, v2s32 dst_pos, v2u32 size, int ratio)
{
	for (u32 y0 = 0; y0 < size.Y; y0++)
	for (u32 x0 = 0; x0 < size.X; x0++)
	{
		s32 src_x = src_pos.X + x0;
		s32 src_y = src_pos.Y + y0;
		s32 dst_x = dst_pos.X + x0;
		s32 dst_y = dst_pos.Y + y0;
		video::SColor src_c = src->getPixel(src_x, src_y);
		video::SColor dst_c = dst->getPixel(dst_x, dst_y);
		if (dst_c.getAlpha() > 0 && src_c.getAlpha() != 0)
		{
			if (ratio == -1)
				dst_c = src_c.getInterpolated(dst_c, (float)src_c.getAlpha()/255.0f);
			else
				dst_c = src_c.getInterpolated(dst_c, (float)ratio/255.0f);
			dst->setPixel(dst_x, dst_y, dst_c);
		}
	}
}
#endif

/*
	Apply color to destination
*/
static void apply_colorize(video::IImage *dst, v2u32 dst_pos, v2u32 size,
		const video::SColor &color, int ratio, bool keep_alpha)
{
	u32 alpha = color.getAlpha();
	video::SColor dst_c;
	if ((ratio == -1 && alpha == 255) || ratio == 255) { // full replacement of color
		if (keep_alpha) { // replace the color with alpha = dest alpha * color alpha
			dst_c = color;
			for (u32 y = dst_pos.Y; y < dst_pos.Y + size.Y; y++)
			for (u32 x = dst_pos.X; x < dst_pos.X + size.X; x++) {
				u32 dst_alpha = dst->getPixel(x, y).getAlpha();
				if (dst_alpha > 0) {
					dst_c.setAlpha(dst_alpha * alpha / 255);
					dst->setPixel(x, y, dst_c);
				}
			}
		} else { // replace the color including the alpha
			for (u32 y = dst_pos.Y; y < dst_pos.Y + size.Y; y++)
			for (u32 x = dst_pos.X; x < dst_pos.X + size.X; x++)
				if (dst->getPixel(x, y).getAlpha() > 0)
					dst->setPixel(x, y, color);
		}
	} else {  // interpolate between the color and destination
		float interp = (ratio == -1 ? color.getAlpha() / 255.0f : ratio / 255.0f);
		for (u32 y = dst_pos.Y; y < dst_pos.Y + size.Y; y++)
		for (u32 x = dst_pos.X; x < dst_pos.X + size.X; x++) {
			dst_c = dst->getPixel(x, y);
			if (dst_c.getAlpha() > 0) {
				dst_c = color.getInterpolated(dst_c, interp);
				dst->setPixel(x, y, dst_c);
			}
		}
	}
}

/*
	Apply color to destination
*/
static void apply_multiplication(video::IImage *dst, v2u32 dst_pos, v2u32 size,
		const video::SColor &color)
{
	video::SColor dst_c;

	for (u32 y = dst_pos.Y; y < dst_pos.Y + size.Y; y++)
	for (u32 x = dst_pos.X; x < dst_pos.X + size.X; x++) {
		dst_c = dst->getPixel(x, y);
		dst_c.set(
				dst_c.getAlpha(),
				(dst_c.getRed() * color.getRed()) / 255,
				(dst_c.getGreen() * color.getGreen()) / 255,
				(dst_c.getBlue() * color.getBlue()) / 255
				);
		dst->setPixel(x, y, dst_c);
	}
}

/*
	Apply mask to destination
*/
static void apply_mask(video::IImage *mask, video::IImage *dst,
		v2s32 mask_pos, v2s32 dst_pos, v2u32 size)
{
	for (u32 y0 = 0; y0 < size.Y; y0++) {
		for (u32 x0 = 0; x0 < size.X; x0++) {
			s32 mask_x = x0 + mask_pos.X;
			s32 mask_y = y0 + mask_pos.Y;
			s32 dst_x = x0 + dst_pos.X;
			s32 dst_y = y0 + dst_pos.Y;
			video::SColor mask_c = mask->getPixel(mask_x, mask_y);
			video::SColor dst_c = dst->getPixel(dst_x, dst_y);
			dst_c.color &= mask_c.color;
			dst->setPixel(dst_x, dst_y, dst_c);
		}
	}
}

static void draw_crack(video::IImage *crack, video::IImage *dst,
		bool use_overlay, s32 frame_count, s32 progression,
		video::IVideoDriver *driver)
{
	// Dimension of destination image
	core::dimension2d<u32> dim_dst = dst->getDimension();
	// Dimension of original image
	core::dimension2d<u32> dim_crack = crack->getDimension();
	// Count of crack stages
	s32 crack_count = dim_crack.Height / dim_crack.Width;
	// Limit frame_count
	if (frame_count > (s32) dim_dst.Height)
		frame_count = dim_dst.Height;
	if (frame_count < 1)
		frame_count = 1;
	// Limit progression
	if (progression > crack_count-1)
		progression = crack_count-1;
	// Dimension of a single crack stage
	core::dimension2d<u32> dim_crack_cropped(
		dim_crack.Width,
		dim_crack.Width
	);
	// Dimension of the scaled crack stage,
	// which is the same as the dimension of a single destination frame
	core::dimension2d<u32> dim_crack_scaled(
		dim_dst.Width,
		dim_dst.Height / frame_count
	);
	// Create cropped and scaled crack images
	video::IImage *crack_cropped = driver->createImage(
			video::ECF_A8R8G8B8, dim_crack_cropped);
	video::IImage *crack_scaled = driver->createImage(
			video::ECF_A8R8G8B8, dim_crack_scaled);

	if (crack_cropped && crack_scaled)
	{
		// Crop crack image
		v2s32 pos_crack(0, progression*dim_crack.Width);
		crack->copyTo(crack_cropped,
				v2s32(0,0),
				core::rect<s32>(pos_crack, dim_crack_cropped));
		// Scale crack image by copying
		crack_cropped->copyToScaling(crack_scaled);
		// Copy or overlay crack image onto each frame
		for (s32 i = 0; i < frame_count; ++i)
		{
			v2s32 dst_pos(0, dim_crack_scaled.Height * i);
			if (use_overlay)
			{
				blit_with_alpha_overlay(crack_scaled, dst,
						v2s32(0,0), dst_pos,
						dim_crack_scaled);
			}
			else
			{
				blit_with_alpha(crack_scaled, dst,
						v2s32(0,0), dst_pos,
						dim_crack_scaled);
			}
		}
	}

	if (crack_scaled)
		crack_scaled->drop();

	if (crack_cropped)
		crack_cropped->drop();
}

void brighten(video::IImage *image)
{
	if (image == NULL)
		return;

	core::dimension2d<u32> dim = image->getDimension();

	for (u32 y=0; y<dim.Height; y++)
	for (u32 x=0; x<dim.Width; x++)
	{
		video::SColor c = image->getPixel(x,y);
		c.setRed(0.5 * 255 + 0.5 * (float)c.getRed());
		c.setGreen(0.5 * 255 + 0.5 * (float)c.getGreen());
		c.setBlue(0.5 * 255 + 0.5 * (float)c.getBlue());
		image->setPixel(x,y,c);
	}
}

u32 parseImageTransform(const std::string& s)
{
	int total_transform = 0;

	std::string transform_names[8];
	transform_names[0] = "i";
	transform_names[1] = "r90";
	transform_names[2] = "r180";
	transform_names[3] = "r270";
	transform_names[4] = "fx";
	transform_names[6] = "fy";

	std::size_t pos = 0;
	while(pos < s.size())
	{
		int transform = -1;
		for (int i = 0; i <= 7; ++i)
		{
			const std::string &name_i = transform_names[i];

			if (s[pos] == ('0' + i))
			{
				transform = i;
				pos++;
				break;
			}
			else if (!(name_i.empty()) &&
				lowercase(s.substr(pos, name_i.size())) == name_i)
			{
				transform = i;
				pos += name_i.size();
				break;
			}
		}
		if (transform < 0)
			break;

		// Multiply total_transform and transform in the group D4
		int new_total = 0;
		if (transform < 4)
			new_total = (transform + total_transform) % 4;
		else
			new_total = (transform - total_transform + 8) % 4;
		if ((transform >= 4) ^ (total_transform >= 4))
			new_total += 4;

		total_transform = new_total;
	}
	return total_transform;
}

core::dimension2d<u32> imageTransformDimension(u32 transform, core::dimension2d<u32> dim)
{
	if (transform % 2 == 0)
		return dim;
	else
		return core::dimension2d<u32>(dim.Height, dim.Width);
}

void imageTransform(u32 transform, video::IImage *src, video::IImage *dst)
{
	if (src == NULL || dst == NULL)
		return;

	core::dimension2d<u32> dstdim = dst->getDimension();

	// Pre-conditions
	assert(dstdim == imageTransformDimension(transform, src->getDimension()));
	assert(transform <= 7);

	/*
		Compute the transformation from source coordinates (sx,sy)
		to destination coordinates (dx,dy).
	*/
	int sxn = 0;
	int syn = 2;
	if (transform == 0)         // identity
		sxn = 0, syn = 2;  //   sx = dx, sy = dy
	else if (transform == 1)    // rotate by 90 degrees ccw
		sxn = 3, syn = 0;  //   sx = (H-1) - dy, sy = dx
	else if (transform == 2)    // rotate by 180 degrees
		sxn = 1, syn = 3;  //   sx = (W-1) - dx, sy = (H-1) - dy
	else if (transform == 3)    // rotate by 270 degrees ccw
		sxn = 2, syn = 1;  //   sx = dy, sy = (W-1) - dx
	else if (transform == 4)    // flip x
		sxn = 1, syn = 2;  //   sx = (W-1) - dx, sy = dy
	else if (transform == 5)    // flip x then rotate by 90 degrees ccw
		sxn = 2, syn = 0;  //   sx = dy, sy = dx
	else if (transform == 6)    // flip y
		sxn = 0, syn = 3;  //   sx = dx, sy = (H-1) - dy
	else if (transform == 7)    // flip y then rotate by 90 degrees ccw
		sxn = 3, syn = 1;  //   sx = (H-1) - dy, sy = (W-1) - dx

	for (u32 dy=0; dy<dstdim.Height; dy++)
	for (u32 dx=0; dx<dstdim.Width; dx++)
	{
		u32 entries[4] = {dx, dstdim.Width-1-dx, dy, dstdim.Height-1-dy};
		u32 sx = entries[sxn];
		u32 sy = entries[syn];
		video::SColor c = src->getPixel(sx,sy);
		dst->setPixel(dx,dy,c);
	}
}

video::ITexture* TextureSource::getNormalTexture(const std::string &name)
{
	if (isKnownSourceImage("override_normal.png"))
		return getTexture("override_normal.png");
	std::string fname_base = name;
	static const char *normal_ext = "_normal.png";
	static const u32 normal_ext_size = strlen(normal_ext);
	size_t pos = fname_base.find(".");
	std::string fname_normal = fname_base.substr(0, pos) + normal_ext;
	if (isKnownSourceImage(fname_normal)) {
		// look for image extension and replace it
		size_t i = 0;
		while ((i = fname_base.find(".", i)) != std::string::npos) {
			fname_base.replace(i, 4, normal_ext);
			i += normal_ext_size;
		}
		return getTexture(fname_base);
	}
	return NULL;
}

video::SColor TextureSource::getTextureAverageColor(const std::string &name)
{
	video::IVideoDriver *driver = m_device->getVideoDriver();
	video::SColor c(0, 0, 0, 0);
	video::ITexture *texture = getTexture(name);
	video::IImage *image = driver->createImage(texture,
		core::position2d<s32>(0, 0),
		texture->getOriginalSize());
	u32 total = 0;
	u32 tR = 0;
	u32 tG = 0;
	u32 tB = 0;
	core::dimension2d<u32> dim = image->getDimension();
	u16 step = 1;
	if (dim.Width > 16)
		step = dim.Width / 16;
	for (u16 x = 0; x < dim.Width; x += step) {
		for (u16 y = 0; y < dim.Width; y += step) {
			c = image->getPixel(x,y);
			if (c.getAlpha() > 0) {
				total++;
				tR += c.getRed();
				tG += c.getGreen();
				tB += c.getBlue();
			}
		}
	}
	image->drop();
	if (total > 0) {
		c.setRed(tR / total);
		c.setGreen(tG / total);
		c.setBlue(tB / total);
	}
	c.setAlpha(255);
	return c;
}


video::ITexture *TextureSource::getShaderFlagsTexture(bool normalmap_present)
{
	std::string tname = "__shaderFlagsTexture";
	tname += normalmap_present ? "1" : "0";

	if (isKnownSourceImage(tname)) {
		return getTexture(tname);
	} else {
		video::IVideoDriver *driver = m_device->getVideoDriver();
		video::IImage *flags_image = driver->createImage(
			video::ECF_A8R8G8B8, core::dimension2d<u32>(1, 1));
		sanity_check(flags_image != NULL);
		video::SColor c(255, normalmap_present ? 255 : 0, 0, 0);
		flags_image->setPixel(0, 0, c);
		insertSourceImage(tname, flags_image);
		flags_image->drop();
		return getTexture(tname);
	}
}
