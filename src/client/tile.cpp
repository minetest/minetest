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

#include <algorithm>
#include <ICameraSceneNode.h>
#include <IVideoDriver.h>
#include "util/string.h"
#include "util/container.h"
#include "util/thread.h"
#include "filesys.h"
#include "settings.h"
#include "mesh.h"
#include "gamedef.h"
#include "util/strfnd.h"
#include "imagefilters.h"
#include "guiscalingfilter.h"
#include "renderingengine.h"
#include "util/base64.h"

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
	const char *extensions[] = { "png", "jpg", "bmp", "tga", NULL };
	// If there is no extension, assume PNG
	if (removeStringEnd(path, extensions).empty())
		path = path + ".png";
	// Check paths until something is found to exist
	const char **ext = extensions;
	do{
		bool r = replace_ext(path, *ext);
		if (!r)
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
std::string getTexturePath(const std::string &filename, bool *is_base_pack)
{
	std::string fullpath;

	// This can set a wrong value on cached textures, but is irrelevant because
	// is_base_pack is only passed when initializing the textures the first time
	if (is_base_pack)
		*is_base_pack = false;
	/*
		Check from cache
	*/
	bool incache = g_texturename_to_path_cache.get(filename, &fullpath);
	if (incache)
		return fullpath;

	/*
		Check from texture_path
	*/
	for (const auto &path : getTextureDirs()) {
		std::string testpath = path + DIR_DELIM;
		testpath.append(filename);
		// Check all filename extensions. Returns "" if not found.
		fullpath = getImagePath(testpath);
		if (!fullpath.empty())
			break;
	}

	/*
		Check from default data directory
	*/
	if (fullpath.empty())
	{
		std::string base_path = porting::path_share + DIR_DELIM + "textures"
				+ DIR_DELIM + "base" + DIR_DELIM + "pack";
		std::string testpath = base_path + DIR_DELIM + filename;
		// Check all filename extensions. Returns "" if not found.
		fullpath = getImagePath(testpath);
		if (is_base_pack && !fullpath.empty())
			*is_base_pack = true;
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
	std::set<std::string> sourceImages;

	TextureInfo(
			const std::string &name_,
			video::ITexture *texture_=NULL
		):
		name(name_),
		texture(texture_)
	{
	}

	TextureInfo(
			const std::string &name_,
			video::ITexture *texture_,
			std::set<std::string> &sourceImages_
		):
		name(name_),
		texture(texture_),
		sourceImages(sourceImages_)
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
		for (auto &m_image : m_images) {
			m_image.second->drop();
		}
		m_images.clear();
	}
	void insert(const std::string &name, video::IImage *img, bool prefer_local)
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
		if (prefer_local) {
			bool is_base_pack;
			std::string path = getTexturePath(name, &is_base_pack);
			// Ignore base pack
			if (!path.empty() && !is_base_pack) {
				video::IImage *img2 = RenderingEngine::get_video_driver()->
					createImageFromFile(path.c_str());
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
	video::IImage *getOrLoad(const std::string &name)
	{
		std::map<std::string, video::IImage*>::iterator n;
		n = m_images.find(name);
		if (n != m_images.end()){
			n->second->grab(); // Grab for caller
			return n->second;
		}
		video::IVideoDriver *driver = RenderingEngine::get_video_driver();
		std::string path = getTexturePath(name);
		if (path.empty()) {
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
	TextureSource();
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

	bool isKnownSourceImage(const std::string &name)
	{
		bool is_known = false;
		bool cache_found = m_source_image_existence.get(name, &is_known);
		if (cache_found)
			return is_known;
		// Not found in cache; find out if a local file exists
		is_known = (!getTexturePath(name).empty());
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

	video::ITexture* getNormalTexture(const std::string &name);
	video::SColor getTextureAverageColor(const std::string &name);
	video::ITexture *getShaderFlagsTexture(bool normamap_present);

private:

	// The id of the thread that is allowed to use irrlicht directly
	std::thread::id m_main_thread;

	// Cache of source images
	// This should be only accessed from the main thread
	SourceImageCache m_sourcecache;

	// Rebuild images and textures from the current set of source images
	// Shall be called from the main thread.
	// You ARE expected to be holding m_textureinfo_cache_mutex
	void rebuildTexture(video::IVideoDriver *driver, TextureInfo &ti);

	// Generate a texture
	u32 generateTexture(const std::string &name);

	// Generate image based on a string like "stone.png" or "[crack:1:0".
	// if baseimg is NULL, it is created. Otherwise stuff is made on it.
	// source_image_names is important to determine when to flush the image from a cache (dynamic media)
	bool generateImagePart(std::string part_of_name, video::IImage *& baseimg, std::set<std::string> &source_image_names);

	/*! Generates an image from a full string like
	 * "stone.png^mineral_coal.png^[crack:1:0".
	 * Shall be called from the main thread.
	 * The returned Image should be dropped.
	 * source_image_names is important to determine when to flush the image from a cache (dynamic media)
	 */
	video::IImage* generateImage(const std::string &name, std::set<std::string> &source_image_names);

	// Thread-safe cache of what source images are known (true = known)
	MutexedMap<std::string, bool> m_source_image_existence;

	// A texture id is index in this array.
	// The first position contains a NULL texture.
	std::vector<TextureInfo> m_textureinfo_cache;
	// Maps a texture name to an index in the former.
	std::map<std::string, u32> m_name_to_id;
	// The two former containers are behind this mutex
	std::mutex m_textureinfo_cache_mutex;

	// Queued texture fetches (to be processed by the main thread)
	RequestQueue<std::string, u32, std::thread::id, u8> m_get_texture_queue;

	// Textures that have been overwritten with other ones
	// but can't be deleted because the ITexture* might still be used
	std::vector<video::ITexture*> m_texture_trash;

	// Maps image file names to loaded palettes.
	std::unordered_map<std::string, Palette> m_palettes;

	// Cached settings needed for making textures from meshes
	bool m_setting_mipmap;
	bool m_setting_trilinear_filter;
	bool m_setting_bilinear_filter;
	bool m_setting_anisotropic_filter;
};

IWritableTextureSource *createTextureSource()
{
	return new TextureSource();
}

TextureSource::TextureSource()
{
	m_main_thread = std::this_thread::get_id();

	// Add a NULL TextureInfo as the first index, named ""
	m_textureinfo_cache.emplace_back("");
	m_name_to_id[""] = 0;

	// Cache some settings
	// Note: Since this is only done once, the game must be restarted
	// for these settings to take effect
	m_setting_mipmap = g_settings->getBool("mip_map");
	m_setting_trilinear_filter = g_settings->getBool("trilinear_filter");
	m_setting_bilinear_filter = g_settings->getBool("bilinear_filter");
	m_setting_anisotropic_filter = g_settings->getBool("anisotropic_filter");
}

TextureSource::~TextureSource()
{
	video::IVideoDriver *driver = RenderingEngine::get_video_driver();

	unsigned int textures_before = driver->getTextureCount();

	for (const auto &iter : m_textureinfo_cache) {
		//cleanup texture
		if (iter.texture)
			driver->removeTexture(iter.texture);
	}
	m_textureinfo_cache.clear();

	for (auto t : m_texture_trash) {
		//cleanup trashed texture
		driver->removeTexture(t);
	}

	infostream << "~TextureSource() before cleanup: "<< textures_before
			<< " after: " << driver->getTextureCount() << std::endl;
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
	if (std::this_thread::get_id() == m_main_thread) {
		return generateTexture(name);
	}


	infostream<<"getTextureId(): Queued: name=\""<<name<<"\""<<std::endl;

	// We're gonna ask the result to be put into here
	static thread_local ResultQueue<std::string, u32, std::thread::id, u8> result_queue;

	// Throw a request in
	m_get_texture_queue.add(name, std::this_thread::get_id(), 0, &result_queue);

	try {
		while(true) {
			// Wait for result for up to 1 seconds (empirical value)
			GetResult<std::string, u32, std::thread::id, u8>
				result = result_queue.pop_front(1000);

			if (result.key == name) {
				return result.item;
			}
		}
	} catch(ItemNotFoundException &e) {
		errorstream << "Waiting for texture " << name << " timed out." << std::endl;
		return 0;
	}

	infostream << "getTextureId(): Failed" << std::endl;

	return 0;
}

// Draw an image on top of another one, using the alpha channel of the
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

// Perform a Screen blend with the given color. The opposite effect of a
// Multiply blend.
static void apply_screen(video::IImage *dst, v2u32 dst_pos, v2u32 size,
		const video::SColor &color);

// Adjust the hue, saturation, and lightness of destination. Like
// "Hue-Saturation" in GIMP.
// If colorize is true then the image will be converted to a grayscale
// image as though seen through a colored glass, like "Colorize" in GIMP.
static void apply_hue_saturation(video::IImage *dst, v2u32 dst_pos, v2u32 size,
		s32 hue, s32 saturation, s32 lightness, bool colorize);

// Apply an overlay blend to an images.
// Overlay blend combines Multiply and Screen blend modes.The parts of the top
// layer where the base layer is light become lighter, the parts where the base
// layer is dark become darker.Areas where the base layer are mid grey are
// unaffected.An overlay with the same picture looks like an S - curve.
static void apply_overlay(video::IImage *overlay, video::IImage *dst,
		v2s32 overlay_pos, v2s32 dst_pos, v2u32 size, bool hardlight);

// Adjust the brightness and contrast of the base image. Conceptually like
// "Brightness-Contrast" in GIMP but allowing brightness to be wound all the
// way up to white or down to black.
static void apply_brightness_contrast(video::IImage *dst, v2u32 dst_pos, v2u32 size,
		s32 brightness, s32 contrast);

// Apply a mask to an image
static void apply_mask(video::IImage *mask, video::IImage *dst,
		v2s32 mask_pos, v2s32 dst_pos, v2u32 size);

// Draw or overlay a crack
static void draw_crack(video::IImage *crack, video::IImage *dst,
		bool use_overlay, s32 frame_count, s32 progression,
		video::IVideoDriver *driver, u8 tiles = 1);

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
	if (name.empty()) {
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
	if (std::this_thread::get_id() != m_main_thread) {
		errorstream<<"TextureSource::generateTexture() "
				"called not from main thread"<<std::endl;
		return 0;
	}

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	sanity_check(driver);

	// passed into texture info for dynamic media tracking
	std::set<std::string> source_image_names;
	video::IImage *img = generateImage(name, source_image_names);

	video::ITexture *tex = NULL;

	if (img != NULL) {
		img = Align2Npot2(img, driver);
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
	TextureInfo ti(name, tex, source_image_names);
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
	// Avoid duplicating texture if it won't actually change
	const bool filter_needed =
		m_setting_mipmap || m_setting_trilinear_filter ||
		m_setting_bilinear_filter || m_setting_anisotropic_filter;
	if (filter_needed)
		return getTexture(name + "^[applyfiltersformesh", id);
	return getTexture(name, id);
}

Palette* TextureSource::getPalette(const std::string &name)
{
	// Only the main thread may load images
	sanity_check(std::this_thread::get_id() == m_main_thread);

	if (name.empty())
		return NULL;

	auto it = m_palettes.find(name);
	if (it == m_palettes.end()) {
		// Create palette
		std::set<std::string> source_image_names; // unused, sadly.
		video::IImage *img = generateImage(name, source_image_names);
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
			new_palette.emplace_back(0xFFFFFFFF);
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
	// NOTE: process outstanding requests from all mesh generation threads
	while (!m_get_texture_queue.empty())
	{
		GetRequest<std::string, u32, std::thread::id, u8>
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

	sanity_check(std::this_thread::get_id() == m_main_thread);

	m_sourcecache.insert(name, img, true);
	m_source_image_existence.set(name, true);

	// now we need to check for any textures that need updating
	MutexAutoLock lock(m_textureinfo_cache_mutex);

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	sanity_check(driver);

	// Recreate affected textures
	u32 affected = 0;
	for (TextureInfo &ti : m_textureinfo_cache) {
		if (ti.name.empty())
			continue; // Skip dummy entry
		// If the source image was used, we need to rebuild this texture
		if (ti.sourceImages.find(name) != ti.sourceImages.end()) {
			rebuildTexture(driver, ti);
			affected++;
		}
	}
	if (affected > 0)
		verbosestream << "TextureSource: inserting \"" << name << "\" caused rebuild of " << affected << " textures." << std::endl;
}

void TextureSource::rebuildImagesAndTextures()
{
	MutexAutoLock lock(m_textureinfo_cache_mutex);

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	sanity_check(driver);

	infostream << "TextureSource: recreating " << m_textureinfo_cache.size()
		<< " textures" << std::endl;

	// Recreate textures
	for (TextureInfo &ti : m_textureinfo_cache) {
		if (ti.name.empty())
			continue; // Skip dummy entry
		rebuildTexture(driver, ti);
	}
}

void TextureSource::rebuildTexture(video::IVideoDriver *driver, TextureInfo &ti)
{
	if (ti.name.empty())
		return; // this shouldn't happen, just a precaution

	// replaces the previous sourceImages
	// shouldn't really need to be done, but can't hurt
	std::set<std::string> source_image_names;
	video::IImage *img = generateImage(ti.name, source_image_names);
	img = Align2Npot2(img, driver);
	// Create texture from resulting image
	video::ITexture *t = NULL;
	if (img) {
		t = driver->addTexture(ti.name.c_str(), img);
		guiScalingCache(io::path(ti.name.c_str()), driver, img);
		img->drop();
	}
	video::ITexture *t_old = ti.texture;
	// Replace texture
	ti.texture = t;
	ti.sourceImages = source_image_names;

	if (t_old)
		m_texture_trash.push_back(t_old);
}

inline static void applyShadeFactor(video::SColor &color, u32 factor)
{
	u32 f = core::clamp<u32>(factor, 0, 256);
	color.setRed(color.getRed() * f / 256);
	color.setGreen(color.getGreen() * f / 256);
	color.setBlue(color.getBlue() * f / 256);
}

static video::IImage *createInventoryCubeImage(
	video::IImage *top, video::IImage *left, video::IImage *right)
{
	core::dimension2du size_top = top->getDimension();
	core::dimension2du size_left = left->getDimension();
	core::dimension2du size_right = right->getDimension();

	u32 size = npot2(std::max({
			size_top.Width, size_top.Height,
			size_left.Width, size_left.Height,
			size_right.Width, size_right.Height,
	}));

	// It must be divisible by 4, to let everything work correctly.
	// But it is a power of 2, so being at least 4 is the same.
	// And the resulting texture should't be too large as well.
	size = core::clamp<u32>(size, 4, 64);

	// With such parameters, the cube fits exactly, touching each image line
	// from `0` to `cube_size - 1`. (Note that division is exact here).
	u32 cube_size = 9 * size;
	u32 offset = size / 2;

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();

	auto lock_image = [size, driver] (video::IImage *&image) -> const u32 * {
		image->grab();
		core::dimension2du dim = image->getDimension();
		video::ECOLOR_FORMAT format = image->getColorFormat();
		if (dim.Width != size || dim.Height != size || format != video::ECF_A8R8G8B8) {
			video::IImage *scaled = driver->createImage(video::ECF_A8R8G8B8, {size, size});
			image->copyToScaling(scaled);
			image->drop();
			image = scaled;
		}
		sanity_check(image->getPitch() == 4 * size);
		return reinterpret_cast<u32 *>(image->getData());
	};
	auto free_image = [] (video::IImage *image) -> void {
		image->drop();
	};

	video::IImage *result = driver->createImage(video::ECF_A8R8G8B8, {cube_size, cube_size});
	sanity_check(result->getPitch() == 4 * cube_size);
	result->fill(video::SColor(0x00000000u));
	u32 *target = reinterpret_cast<u32 *>(result->getData());

	// Draws single cube face
	// `shade_factor` is face brightness, in range [0.0, 1.0]
	// (xu, xv, x1; yu, yv, y1) form coordinate transformation matrix
	// `offsets` list pixels to be drawn for single source pixel
	auto draw_image = [=] (video::IImage *image, float shade_factor,
			s16 xu, s16 xv, s16 x1,
			s16 yu, s16 yv, s16 y1,
			std::initializer_list<v2s16> offsets) -> void {
		u32 brightness = core::clamp<u32>(256 * shade_factor, 0, 256);
		const u32 *source = lock_image(image);
		for (u16 v = 0; v < size; v++) {
			for (u16 u = 0; u < size; u++) {
				video::SColor pixel(*source);
				applyShadeFactor(pixel, brightness);
				s16 x = xu * u + xv * v + x1;
				s16 y = yu * u + yv * v + y1;
				for (const auto &off : offsets)
					target[(y + off.Y) * cube_size + (x + off.X) + offset] = pixel.color;
				source++;
			}
		}
		free_image(image);
	};

	draw_image(top, 1.000000f,
			4, -4, 4 * (size - 1),
			2, 2, 0,
			{
				                {2, 0}, {3, 0}, {4, 0}, {5, 0},
				{0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1},
				                {2, 2}, {3, 2}, {4, 2}, {5, 2},
			});

	draw_image(left, 0.836660f,
			4, 0, 0,
			2, 5, 2 * size,
			{
				{0, 0}, {1, 0},
				{0, 1}, {1, 1}, {2, 1}, {3, 1},
				{0, 2}, {1, 2}, {2, 2}, {3, 2},
				{0, 3}, {1, 3}, {2, 3}, {3, 3},
				{0, 4}, {1, 4}, {2, 4}, {3, 4},
				                {2, 5}, {3, 5},
			});

	draw_image(right, 0.670820f,
			4, 0, 4 * size,
			-2, 5, 4 * size - 2,
			{
				                {2, 0}, {3, 0},
				{0, 1}, {1, 1}, {2, 1}, {3, 1},
				{0, 2}, {1, 2}, {2, 2}, {3, 2},
				{0, 3}, {1, 3}, {2, 3}, {3, 3},
				{0, 4}, {1, 4}, {2, 4}, {3, 4},
				{0, 5}, {1, 5},
			});

	return result;
}

video::IImage* TextureSource::generateImage(const std::string &name, std::set<std::string> &source_image_names)
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
		baseimg = generateImage(name.substr(0, last_separator_pos), source_image_names);
	}

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
		video::IImage *tmp = generateImage(name2, source_image_names);
		if (!tmp) {
			errorstream << "generateImage(): "
				"Failed to generate \"" << name2 << "\""
				<< std::endl;
			return NULL;
		}

		if (baseimg) {
			core::dimension2d<u32> dim = tmp->getDimension();
			blit_with_alpha(tmp, baseimg, v2s32(0, 0), v2s32(0, 0), dim);
			tmp->drop();
		} else {
			baseimg = tmp;
		}
	} else if (!generateImagePart(last_part_of_name, baseimg, source_image_names)) {
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

/**
 * Check and align image to npot2 if required by hardware
 * @param image image to check for npot2 alignment
 * @param driver driver to use for image operations
 * @return image or copy of image aligned to npot2
 */
video::IImage *Align2Npot2(video::IImage *image,
		video::IVideoDriver *driver)
{
	if (image == NULL)
		return image;

	if (driver->queryFeature(video::EVDF_TEXTURE_NPOT))
		return image;

	core::dimension2d<u32> dim = image->getDimension();
	unsigned int height = npot2(dim.Height);
	unsigned int width  = npot2(dim.Width);

	if (dim.Height == height && dim.Width == width)
		return image;

	if (dim.Height > height)
		height *= 2;
	if (dim.Width > width)
		width *= 2;

	video::IImage *targetimage =
			driver->createImage(video::ECF_A8R8G8B8,
					core::dimension2d<u32>(width, height));

	if (targetimage != NULL)
		image->copyToScaling(targetimage);
	image->drop();
	return targetimage;
}

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

/*
	Replaces the smaller of the two images with one upscaled to match the
	dimensions of the other.
	Ensure no other references to these images are being held, as one may
	get dropped and switched with a new image.
*/
void upscaleImagesToMatchLargest(video::IImage *& img1,
	video::IImage *& img2)
{
	core::dimension2d<u32> dim1 = img1->getDimension();
	core::dimension2d<u32> dim2 = img2->getDimension();

	if (dim1 == dim2) {
		// image dimensions match, no scaling required

	}
	else if (dim1.Width * dim1.Height < dim2.Width * dim2.Height) {
		// Upscale img1
		video::IImage *scaled_image = RenderingEngine::get_video_driver()->
			createImage(video::ECF_A8R8G8B8, dim2);
		img1->copyToScaling(scaled_image);
		img1->drop();
		img1 = scaled_image;

	} else {
		// Upscale img2
		video::IImage *scaled_image = RenderingEngine::get_video_driver()->
			createImage(video::ECF_A8R8G8B8, dim1);
		img2->copyToScaling(scaled_image);
		img2->drop();
		img2 = scaled_image;
	}
}

void blitBaseImage(video::IImage* &src, video::IImage* &dst)
{
	//infostream<<"Blitting "<<part_of_name<<" on base"<<std::endl;
	upscaleImagesToMatchLargest(dst, src);

	// Size of the copied area
	core::dimension2d<u32> dim_dst = dst->getDimension();
	// Position to copy the blitted to in the base image
	core::position2d<s32> pos_to(0,0);
	// Position to copy the blitted from in the blitted image
	core::position2d<s32> pos_from(0,0);

	blit_with_alpha(src, dst, pos_from, pos_to, dim_dst);
}

bool TextureSource::generateImagePart(std::string part_of_name,
		video::IImage *& baseimg, std::set<std::string> &source_image_names)
{
	const char escape = '\\'; // same as in generateImage()
	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	sanity_check(driver);

	// Stuff starting with [ are special commands
	if (part_of_name.empty() || part_of_name[0] != '[') {
		source_image_names.insert(part_of_name);
		video::IImage *image = m_sourcecache.getOrLoad(part_of_name);
		if (image == NULL) {
			if (!part_of_name.empty()) {

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
			blitBaseImage(image, baseimg);
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
			// Format: crack[o][:<tiles>]:<frame_count>:<frame>
			bool use_overlay = (part_of_name[6] == 'o');
			Strfnd sf(part_of_name);
			sf.next(":");
			s32 frame_count = stoi(sf.next(":"));
			s32 progression = stoi(sf.next(":"));
			s32 tiles = 1;
			// Check whether there is the <tiles> argument, that is,
			// whether there are 3 arguments. If so, shift values
			// as the first and not the last argument is optional.
			auto s = sf.next(":");
			if (!s.empty()) {
				tiles = frame_count;
				frame_count = progression;
				progression = stoi(s);
			}

			if (progression >= 0) {
				/*
					Load crack image.

					It is an image with a number of cracking stages
					horizontally tiled.
				*/
				video::IImage *img_crack = m_sourcecache.getOrLoad(
					"crack_anylength.png");

				if (img_crack) {
					draw_crack(img_crack, baseimg,
						use_overlay, frame_count,
						progression, driver, tiles);
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
			while (!sf.at_end()) {
				u32 x = stoi(sf.next(","));
				u32 y = stoi(sf.next("="));
				std::string filename = unescape_string(sf.next_esc(":", escape), escape);
				infostream<<"Adding \""<<filename
						<<"\" to combined ("<<x<<","<<y<<")"
						<<std::endl;
				video::IImage *img = generateImage(filename, source_image_names);
				if (img) {
					core::dimension2d<u32> dim = img->getDimension();
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
			[fill:WxH:color
			[fill:WxH:X,Y:color
			Creates a texture of the given size and color, optionally with an <x>,<y>
			position. An alpha value may be specified in the `Colorstring`.
		*/
		else if (str_starts_with(part_of_name, "[fill"))
		{
			s32 x = 0;
			s32 y = 0;

			Strfnd sf(part_of_name);
			sf.next(":");
			u32 width  = stoi(sf.next("x"));
			u32 height = stoi(sf.next(":"));
			std::string color_or_x = sf.next(",");

			video::SColor color;
			if (!parseColorString(color_or_x, color, true)) {
				x = stoi(color_or_x);
				y = stoi(sf.next(":"));
				std::string color_str = sf.next(":");

				if (!parseColorString(color_str, color, false))
					return false;
			}
			core::dimension2d<u32> dim(width, height);

			video::IImage *img = driver->createImage(video::ECF_A8R8G8B8, dim);
			img->fill(color);

			if (baseimg == nullptr) {
				baseimg = img;
			} else {
				blit_with_alpha(img, baseimg, v2s32(0, 0), v2s32(x, y), dim);
				img->drop();
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
			video::IImage *img_top = generateImage(imagename_top, source_image_names);
			video::IImage *img_left = generateImage(imagename_left, source_image_names);
			video::IImage *img_right = generateImage(imagename_right, source_image_names);

			if (img_top == NULL || img_left == NULL || img_right == NULL) {
				errorstream << "generateImagePart(): Failed to create textures"
						<< " for inventorycube \"" << part_of_name << "\""
						<< std::endl;
				baseimg = generateImage(imagename_top, source_image_names);
				return true;
			}

			baseimg = createInventoryCubeImage(img_top, img_left, img_right);

			// Face images are not needed anymore
			img_top->drop();
			img_left->drop();
			img_right->drop();

			return true;
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
			video::IImage *img = generateImage(filename, source_image_names);
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

			video::IImage *img = generateImage(filename, source_image_names);
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
		or 
		[screen:color
			Multiply and Screen blend modes are basic blend modes for darkening and lightening
			images, respectively.
			A Multiply blend multiplies a given color to every pixel of an image.
			A Screen blend has the opposite effect to a Multiply blend.
			color = color as ColorString
		*/
		else if (str_starts_with(part_of_name, "[multiply:") ||
		         str_starts_with(part_of_name, "[screen:")) {
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
			if (str_starts_with(part_of_name, "[multiply:")) {
				apply_multiplication(baseimg, v2u32(0, 0), 
					baseimg->getDimension(), color);
			} else {
				apply_screen(baseimg, v2u32(0, 0), baseimg->getDimension(), color);
			}
		}
		/*
			[colorize:color:ratio
			Overlays image with given color
			color = color as ColorString
			ratio = optional string "alpha", or a weighting between 0 and 255
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
			/* IMPORTANT: When changing this, getTextureForMesh() needs to be
			 * updated too. */

			if (!baseimg) {
				errorstream << "generateImagePart(): baseimg == NULL "
						<< "for part_of_name=\"" << part_of_name
						<< "\", cancelling." << std::endl;
				return false;
			}

			// Apply the "clean transparent" filter, if needed
			if (m_setting_mipmap || m_setting_bilinear_filter ||
				m_setting_trilinear_filter || m_setting_anisotropic_filter)
				imageCleanTransparent(baseimg, 127);

			/* Upscale textures to user's requested minimum size.  This is a trick to make
			 * filters look as good on low-res textures as on high-res ones, by making
			 * low-res textures BECOME high-res ones.  This is helpful for worlds that
			 * mix high- and low-res textures, or for mods with least-common-denominator
			 * textures that don't have the resources to offer high-res alternatives.
			 */
			const bool filter = m_setting_trilinear_filter || m_setting_bilinear_filter;
			const s32 scaleto = filter ? g_settings->getU16("texture_min_size") : 1;
			if (scaleto > 1) {
				const core::dimension2d<u32> dim = baseimg->getDimension();

				/* Calculate scaling needed to make the shortest texture dimension
				 * equal to the target minimum.  If e.g. this is a vertical frames
				 * animation, the short dimension will be the real size.
				 */
				if (dim.Width == 0 || dim.Height == 0) {
					errorstream << "generateImagePart(): Illegal 0 dimension "
						<< "for part_of_name=\""<< part_of_name
						<< "\", cancelling." << std::endl;
					return false;
				}
				u32 xscale = scaleto / dim.Width;
				u32 yscale = scaleto / dim.Height;
				const s32 scale = std::max(xscale, yscale);

				// Never downscale; only scale up by 2x or more.
				if (scale > 1) {
					u32 w = scale * dim.Width;
					u32 h = scale * dim.Height;
					const core::dimension2d<u32> newdim(w, h);
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

			video::IImage *image = RenderingEngine::get_video_driver()->
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
			if (mode.find('a') != std::string::npos)
				mask |= 0xff000000UL;
			if (mode.find('r') != std::string::npos)
				mask |= 0x00ff0000UL;
			if (mode.find('g') != std::string::npos)
				mask |= 0x0000ff00UL;
			if (mode.find('b') != std::string::npos)
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
		/*
			[png:base64
			Decodes a PNG image in base64 form.
			Use minetest.encode_png and minetest.encode_base64
			to produce a valid string.
		*/
		else if (str_starts_with(part_of_name, "[png:")) {
			Strfnd sf(part_of_name);
			sf.next(":");
			std::string png;
			{
				std::string blob = sf.next("");
				if (!base64_is_valid(blob)) {
					errorstream << "generateImagePart(): "
								<< "malformed base64 in '[png'"
								<< std::endl;
					return false;
				}
				png = base64_decode(blob);
			}

			auto *device = RenderingEngine::get_raw_device();
			auto *fs = device->getFileSystem();
			auto *vd = device->getVideoDriver();
			auto *memfile = fs->createMemoryReadFile(png.data(), png.size(), "__temp_png");
			video::IImage* pngimg = vd->createImageFromFile(memfile);
			memfile->drop();

			if (!pngimg) {
				errorstream << "generateImagePart(): Invalid PNG data" << std::endl;
				return false;
			}

			if (baseimg) {
				blitBaseImage(pngimg, baseimg);
			} else {
				core::dimension2d<u32> dim = pngimg->getDimension();
				baseimg = driver->createImage(video::ECF_A8R8G8B8, dim);
				pngimg->copyTo(baseimg);
			}
			pngimg->drop();
		}
		/*
			[hsl:hue:saturation:lightness
			or 
			[colorizehsl:hue:saturation:lightness

			Adjust the hue, saturation, and lightness of the base image. Like
			"Hue-Saturation" in GIMP, but with 0 as the mid-point.
			Hue should be from -180 to +180, though 0 to 360 is also supported.
			Saturation and lightness are optional, with lightness from -100 to
			+100, and sauration from -100 to +100-or-higher.

			If colorize is true then saturation is from 0 to 100, and the image
			will be converted to a grayscale image as though seen through a
			colored glass, like	"Colorize" in GIMP.
		*/
		else if (str_starts_with(part_of_name, "[hsl:") || 
		         str_starts_with(part_of_name, "[colorizehsl:")) {

			if (baseimg == nullptr) {
				errorstream << "generateImagePart(): baseimg == NULL "
					<< "for part_of_name=\"" << part_of_name
					<< "\", cancelling." << std::endl;
				return false;
			}

			bool colorize = str_starts_with(part_of_name, "[colorizehsl:");

			// saturation range is 0 to 100 when colorize is true
			s32 defaultSaturation = colorize ? 50 : 0;

			Strfnd sf(part_of_name);
			sf.next(":");
			s32 hue = mystoi(sf.next(":"), -180, 360); 
			s32 saturation = sf.at_end() ? defaultSaturation : mystoi(sf.next(":"), -100, 1000);
			s32 lightness  = sf.at_end() ? 0 : mystoi(sf.next(":"), -100, 100);


			apply_hue_saturation(baseimg, v2u32(0, 0), baseimg->getDimension(),
				hue, saturation, lightness, colorize);
		}
		/*
			[overlay:filename
			or 
			[hardlight:filename

			"A.png^[hardlight:B.png" is the same as "B.png^[overlay:A.Png"

			Applies an Overlay or Hard Light blend between two images, like the
			layer modes of the same names in GIMP.
			Overlay combines Multiply and Screen blend modes. The parts of the
			top layer where the base layer is light become lighter, the parts
			where the base layer is dark become darker. Areas where the base
			layer are mid grey are unaffected. An overlay with the same picture
			looks like an S-curve.

			Swapping the top layer and base layer is a Hard Light blend
		*/
		else if (str_starts_with(part_of_name, "[overlay:") ||
		         str_starts_with(part_of_name, "[hardlight:")) {

			if (baseimg == nullptr) {
				errorstream << "generateImage(): baseimg == NULL "
					<< "for part_of_name=\"" << part_of_name
					<< "\", cancelling." << std::endl;
				return false;
			}
			Strfnd sf(part_of_name);
			sf.next(":");
			std::string filename = unescape_string(sf.next_esc(":", escape), escape);

			video::IImage *img = generateImage(filename, source_image_names);
			if (img) {
				upscaleImagesToMatchLargest(baseimg, img);

				bool hardlight = str_starts_with(part_of_name, "[hardlight:");
				apply_overlay(img, baseimg, v2s32(0, 0), v2s32(0, 0),
						img->getDimension(), hardlight);
				img->drop();
			} else {
				errorstream << "generateImage(): Failed to load \""
					<< filename << "\".";
			}
		}
		/*
			[contrast:C:B

			Adjust the brightness and contrast of the base image. Conceptually
			like GIMP's "Brightness-Contrast" feature but allows brightness to
			be wound all the way up to white or down to black.
			C and B are both values from -127 to +127.
			B is optional.
		*/
		else if (str_starts_with(part_of_name, "[contrast:")) {

			if (baseimg == nullptr) {
				errorstream << "generateImagePart(): baseimg == NULL "
					<< "for part_of_name=\"" << part_of_name
					<< "\", cancelling." << std::endl;
				return false;
			}

			Strfnd sf(part_of_name);
			sf.next(":");
			s32 contrast = mystoi(sf.next(":"), -127, 127);
			s32 brightness = sf.at_end() ? 0 : mystoi(sf.next(":"), -127, 127);

			apply_brightness_contrast(baseimg, v2u32(0, 0), 
				baseimg->getDimension(), brightness, contrast);
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
	Calculate the color of a single pixel drawn on top of another pixel.

	This is a little more complicated than just video::SColor::getInterpolated
	because getInterpolated does not handle alpha correctly.  For example, a
	pixel with alpha=64 drawn atop a pixel with alpha=128 should yield a
	pixel with alpha=160, while getInterpolated would yield alpha=96.
*/
static inline video::SColor blitPixel(const video::SColor &src_c, const video::SColor &dst_c, u32 ratio)
{
	if (dst_c.getAlpha() == 0)
		return src_c;
	video::SColor out_c = src_c.getInterpolated(dst_c, (float)ratio / 255.0f);
	out_c.setAlpha(dst_c.getAlpha() + (255 - dst_c.getAlpha()) *
		src_c.getAlpha() * ratio / (255 * 255));
	return out_c;
}

/*
	Draw an image on top of another one, using the alpha channel of the
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
		dst_c = blitPixel(src_c, dst_c, src_c.getAlpha());
		dst->setPixel(dst_x, dst_y, dst_c);
	}
}

/*
	Draw an image on top of another one, using the alpha channel of the
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
			dst_c = blitPixel(src_c, dst_c, src_c.getAlpha());
			dst->setPixel(dst_x, dst_y, dst_c);
		}
	}
}

// This function has been disabled because it is currently unused.
// Feel free to re-enable if you find it handy.
#if 0
/*
	Draw an image on top of another one, using the specified ratio
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
	Apply color to destination, using a weighted interpolation blend
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
	Apply color to destination, using a Multiply blend mode
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
	Apply color to destination, using a Screen blend mode
*/
static void apply_screen(video::IImage *dst, v2u32 dst_pos, v2u32 size,
		const video::SColor &color)
{
	video::SColor dst_c;

	for (u32 y = dst_pos.Y; y < dst_pos.Y + size.Y; y++)
	for (u32 x = dst_pos.X; x < dst_pos.X + size.X; x++) {
		dst_c = dst->getPixel(x, y);
		dst_c.set(
			dst_c.getAlpha(),
			255 - ((255 - dst_c.getRed())   * (255 - color.getRed()))   / 255,
			255 - ((255 - dst_c.getGreen()) * (255 - color.getGreen())) / 255,
			255 - ((255 - dst_c.getBlue())  * (255 - color.getBlue()))  / 255
		);
		dst->setPixel(x, y, dst_c);
	}
}

/*
	Adjust the hue, saturation, and lightness of destination. Like
	"Hue-Saturation" in GIMP, but with 0 as the mid-point.
	Hue should be from -180 to +180, or from 0 to 360.
	Saturation and Lightness are percentages.
	Lightness is from -100 to +100.
	Saturation goes down to -100 (fully desaturated) but can go above 100,
	allowing for even muted colors to become saturated.

	If colorize is true then saturation is from 0 to 100, and destination will
	be converted to a grayscale image as seen through a colored glass, like
	"Colorize" in GIMP.
*/
static void apply_hue_saturation(video::IImage *dst, v2u32 dst_pos, v2u32 size,
	s32 hue, s32 saturation, s32 lightness, bool colorize)
{
	video::SColorf colorf;
	video::SColorHSL hsl;
	f32 norm_s = core::clamp(saturation, -100, 1000) / 100.0f;
	f32 norm_l = core::clamp(lightness,  -100, 100) / 100.0f;

	if (colorize) {
		hsl.Saturation = core::clamp((f32)saturation, 0.0f, 100.0f);
	}

	for (u32 y = dst_pos.Y; y < dst_pos.Y + size.Y; y++)
		for (u32 x = dst_pos.X; x < dst_pos.X + size.X; x++) {

			if (colorize) {
				f32 lum = dst->getPixel(x, y).getLuminance() / 255.0f;

				if (norm_l < 0) {
					lum *= norm_l + 1.0f;
				} else {
					lum = lum * (1.0f - norm_l) + norm_l;
				}
				hsl.Hue = 0;
				hsl.Luminance = lum * 100;

			} else {
				// convert the RGB to HSL
				colorf = video::SColorf(dst->getPixel(x, y));
				hsl.fromRGB(colorf);

				if (norm_l < 0) {
					hsl.Luminance *= norm_l + 1.0f;
				} else{
					hsl.Luminance = hsl.Luminance + norm_l * (100.0f - hsl.Luminance);
				}

				// Adjusting saturation in the same manner as lightness resulted in
				// muted colours being affected too much and bright colours not
				// affected enough, so I'm borrowing a leaf out of gimp's book and
				// using a different scaling approach for saturation.
				// https://github.com/GNOME/gimp/blob/6cc1e035f1822bf5198e7e99a53f7fa6e281396a/app/operations/gimpoperationhuesaturation.c#L139-L145=
				// This difference is why values over 100% are not necessary for
				// lightness but are very useful with saturation. An alternative UI
				// approach would be to have an upper saturation limit of 100, but
				// multiply positive values by ~3 to make it a more useful positive
				// range scale.
				hsl.Saturation *= norm_s + 1.0f;
				hsl.Saturation = core::clamp(hsl.Saturation, 0.0f, 100.0f);
			}

			// Apply the specified HSL adjustments
			hsl.Hue = fmod(hsl.Hue + hue, 360);
			if (hsl.Hue < 0)
				hsl.Hue += 360;

			// Convert back to RGB
			hsl.toRGB(colorf);
			dst->setPixel(x, y, colorf.toSColor());
		}
}


/*
	Apply an Overlay blend to destination
	If hardlight is true then swap the dst & blend images (a hardlight blend)
*/
static void apply_overlay(video::IImage *blend, video::IImage *dst,
	v2s32 blend_pos, v2s32 dst_pos, v2u32 size, bool hardlight)
{
	video::IImage *blend_layer = hardlight ? dst : blend;
	video::IImage *base_layer  = hardlight ? blend : dst;
	v2s32 blend_layer_pos = hardlight ? dst_pos : blend_pos;
	v2s32 base_layer_pos  = hardlight ? blend_pos : dst_pos;

	for (u32 y = 0; y < size.Y; y++) 
	for (u32 x = 0; x < size.X; x++) {
		s32 base_x = x + base_layer_pos.X;
		s32 base_y = y + base_layer_pos.Y;

		video::SColor blend_c =
			blend_layer->getPixel(x + blend_layer_pos.X, y + blend_layer_pos.Y);
		video::SColor base_c = base_layer->getPixel(base_x, base_y);		
		double blend_r = blend_c.getRed()   / 255.0;
		double blend_g = blend_c.getGreen() / 255.0;
		double blend_b = blend_c.getBlue()  / 255.0;
		double base_r = base_c.getRed()   / 255.0;
		double base_g = base_c.getGreen() / 255.0;
		double base_b = base_c.getBlue()  / 255.0;

		base_c.set(
			base_c.getAlpha(),
			// Do a Multiply blend if less that 0.5, otherwise do a Screen blend
			(u32)((base_r < 0.5 ? 2 * base_r * blend_r : 1 - 2 * (1 - base_r) * (1 - blend_r)) * 255),
			(u32)((base_g < 0.5 ? 2 * base_g * blend_g : 1 - 2 * (1 - base_g) * (1 - blend_g)) * 255),
			(u32)((base_b < 0.5 ? 2 * base_b * blend_b : 1 - 2 * (1 - base_b) * (1 - blend_b)) * 255)
		);
		dst->setPixel(base_x, base_y, base_c);
	}
}

/* 
	Adjust the brightness and contrast of the base image.

	Conceptually like GIMP's "Brightness-Contrast" feature but allows brightness to be
	wound all the way up to white or down to black.
*/
static void apply_brightness_contrast(video::IImage *dst, v2u32 dst_pos, v2u32 size,
	s32 brightness, s32 contrast)
{
	video::SColor dst_c;
	// Only allow normalized contrast to get as high as 127/128 to avoid infinite slope.
	// (we could technically allow -128/128 here as that would just result in 0 slope)
	double norm_c = core::clamp(contrast,   -127, 127) / 128.0;
	double norm_b = core::clamp(brightness, -127, 127) / 127.0;
	
	// Scale brightness so its range is -127.5 to 127.5, otherwise brightness
	// adjustments will outputs values from 0.5 to 254.5 instead of 0 to 255.
	double scaled_b = brightness * 127.5 / 127;

	// Calculate a contrast slope such that that no colors will get clamped due 
	// to the brightness setting.
	// This allows the texture modifier to used as a brightness modifier without
	// the user having to calculate a contrast to avoid clipping at that brightness.
	double slope = 1 - fabs(norm_b);
	
	// Apply the user's contrast adjustment to the calculated slope, such that
	// -127 will make it near-vertical and +127 will make it horizontal
	double angle = atan(slope);
	angle += norm_c <= 0
		? norm_c * angle // allow contrast slope to be lowered to 0
		: norm_c * (M_PI_2 - angle); // allow contrast slope to be raised almost vert.
	slope = tan(angle);

	double c = slope <= 1
		? -slope * 127.5 + 127.5 + scaled_b    // shift up/down when slope is horiz.
		: -slope * (127.5 - scaled_b) + 127.5; // shift left/right when slope is vert.

	// add 0.5 to c so that when the final result is cast to int, it is effectively
	// rounded rather than trunc'd.
	c += 0.5;

	for (u32 y = dst_pos.Y; y < dst_pos.Y + size.Y; y++)
	for (u32 x = dst_pos.X; x < dst_pos.X + size.X; x++) {
		dst_c = dst->getPixel(x, y);

		dst_c.set(
			dst_c.getAlpha(),
			core::clamp((int)(slope * dst_c.getRed()   + c), 0, 255),
			core::clamp((int)(slope * dst_c.getGreen() + c), 0, 255),
			core::clamp((int)(slope * dst_c.getBlue()  + c), 0, 255)
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

video::IImage *create_crack_image(video::IImage *crack, s32 frame_index,
		core::dimension2d<u32> size, u8 tiles, video::IVideoDriver *driver)
{
	core::dimension2d<u32> strip_size = crack->getDimension();
	core::dimension2d<u32> frame_size(strip_size.Width, strip_size.Width);
	core::dimension2d<u32> tile_size(size / tiles);
	s32 frame_count = strip_size.Height / strip_size.Width;
	if (frame_index >= frame_count)
		frame_index = frame_count - 1;
	core::rect<s32> frame(v2s32(0, frame_index * frame_size.Height), frame_size);
	video::IImage *result = nullptr;

// extract crack frame
	video::IImage *crack_tile = driver->createImage(video::ECF_A8R8G8B8, tile_size);
	if (!crack_tile)
		return nullptr;
	if (tile_size == frame_size) {
		crack->copyTo(crack_tile, v2s32(0, 0), frame);
	} else {
		video::IImage *crack_frame = driver->createImage(video::ECF_A8R8G8B8, frame_size);
		if (!crack_frame)
			goto exit__has_tile;
		crack->copyTo(crack_frame, v2s32(0, 0), frame);
		crack_frame->copyToScaling(crack_tile);
		crack_frame->drop();
	}
	if (tiles == 1)
		return crack_tile;

// tile it
	result = driver->createImage(video::ECF_A8R8G8B8, size);
	if (!result)
		goto exit__has_tile;
	result->fill({});
	for (u8 i = 0; i < tiles; i++)
		for (u8 j = 0; j < tiles; j++)
			crack_tile->copyTo(result, v2s32(i * tile_size.Width, j * tile_size.Height));

exit__has_tile:
	crack_tile->drop();
	return result;
}

static void draw_crack(video::IImage *crack, video::IImage *dst,
		bool use_overlay, s32 frame_count, s32 progression,
		video::IVideoDriver *driver, u8 tiles)
{
	// Dimension of destination image
	core::dimension2d<u32> dim_dst = dst->getDimension();
	// Limit frame_count
	if (frame_count > (s32) dim_dst.Height)
		frame_count = dim_dst.Height;
	if (frame_count < 1)
		frame_count = 1;
	// Dimension of the scaled crack stage,
	// which is the same as the dimension of a single destination frame
	core::dimension2d<u32> frame_size(
		dim_dst.Width,
		dim_dst.Height / frame_count
	);
	video::IImage *crack_scaled = create_crack_image(crack, progression,
			frame_size, tiles, driver);
	if (!crack_scaled)
		return;

	auto blit = use_overlay ? blit_with_alpha_overlay : blit_with_alpha;
	for (s32 i = 0; i < frame_count; ++i) {
		v2s32 dst_pos(0, frame_size.Height * i);
		blit(crack_scaled, dst, v2s32(0,0), dst_pos, frame_size);
	}

	crack_scaled->drop();
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

			if (!(name_i.empty()) && lowercase(s.substr(pos, name_i.size())) == name_i) {
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
	size_t pos = fname_base.find('.');
	std::string fname_normal = fname_base.substr(0, pos) + normal_ext;
	if (isKnownSourceImage(fname_normal)) {
		// look for image extension and replace it
		size_t i = 0;
		while ((i = fname_base.find('.', i)) != std::string::npos) {
			fname_base.replace(i, 4, normal_ext);
			i += normal_ext_size;
		}
		return getTexture(fname_base);
	}
	return NULL;
}

namespace {
	// For more colourspace transformations, see for example
	// https://github.com/tobspr/GLSL-Color-Spaces/blob/master/ColorSpaces.inc.glsl

	inline float linear_to_srgb_component(float v)
	{
		if (v > 0.0031308f)
			return 1.055f * powf(v, 1.0f / 2.4f) - 0.055f;
		return 12.92f * v;
	}
	inline float srgb_to_linear_component(float v)
	{
		if (v > 0.04045f)
			return powf((v + 0.055f) / 1.055f, 2.4f);
		return v / 12.92f;
	}

	v3f srgb_to_linear(const video::SColor &col_srgb)
	{
		v3f col(col_srgb.getRed(), col_srgb.getGreen(), col_srgb.getBlue());
		col /= 255.0f;
		col.X = srgb_to_linear_component(col.X);
		col.Y = srgb_to_linear_component(col.Y);
		col.Z = srgb_to_linear_component(col.Z);
		return col;
	}

	video::SColor linear_to_srgb(const v3f &col_linear)
	{
		v3f col;
		col.X = linear_to_srgb_component(col_linear.X);
		col.Y = linear_to_srgb_component(col_linear.Y);
		col.Z = linear_to_srgb_component(col_linear.Z);
		col *= 255.0f;
		col.X = core::clamp<float>(col.X, 0.0f, 255.0f);
		col.Y = core::clamp<float>(col.Y, 0.0f, 255.0f);
		col.Z = core::clamp<float>(col.Z, 0.0f, 255.0f);
		return video::SColor(0xff, myround(col.X), myround(col.Y),
			myround(col.Z));
	}
}

video::SColor TextureSource::getTextureAverageColor(const std::string &name)
{
	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	video::SColor c(0, 0, 0, 0);
	video::ITexture *texture = getTexture(name);
	if (!texture)
		return c;
	video::IImage *image = driver->createImage(texture,
		core::position2d<s32>(0, 0),
		texture->getOriginalSize());
	if (!image)
		return c;

	u32 total = 0;
	v3f col_acc(0, 0, 0);
	core::dimension2d<u32> dim = image->getDimension();
	u16 step = 1;
	if (dim.Width > 16)
		step = dim.Width / 16;
	for (u16 x = 0; x < dim.Width; x += step) {
		for (u16 y = 0; y < dim.Width; y += step) {
			c = image->getPixel(x,y);
			if (c.getAlpha() > 0) {
				total++;
				col_acc += srgb_to_linear(c);
			}
		}
	}
	image->drop();
	if (total > 0) {
		col_acc /= total;
		c = linear_to_srgb(col_acc);
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
	}

	video::IVideoDriver *driver = RenderingEngine::get_video_driver();
	video::IImage *flags_image = driver->createImage(
		video::ECF_A8R8G8B8, core::dimension2d<u32>(1, 1));
	sanity_check(flags_image != NULL);
	video::SColor c(255, normalmap_present ? 255 : 0, 0, 0);
	flags_image->setPixel(0, 0, c);
	insertSourceImage(tname, flags_image);
	flags_image->drop();
	return getTexture(tname);

}

std::vector<std::string> getTextureDirs()
{
	return fs::GetRecursiveDirs(g_settings->get("texture_path"));
}
