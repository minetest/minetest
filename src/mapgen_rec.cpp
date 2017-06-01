/*
Minetest
Copyright (C) 2016-2017 MillersMan <millersman@users.noreply.github.com>

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

#include "mapgen_rec.h"

#include "emerge.h"
#include "map.h"

#include <iostream>

RecursiveMapgen::RecursiveMapgen(int mapgenid, MapgenParams* params, EmergeManager* emerge)
	: Mapgen(mapgenid, params, emerge)
	, root_level(8)
//    , grid_origin(-32768, -32768)
	, grid_origin(0, 0)
{
}

void RecursiveMapgen::generateAndBindPosition(s16 x, s16 z)
{
	s32 frag_x = (x - grid_origin.X) / frag_size * frag_size;
	s32 frag_z = (z - grid_origin.Y) / frag_size * frag_size;

	RecursionFragment* frag = getFragment(frag_x, frag_z);

	// Bind the fragment to the mapgen buffers
	BufferIfaceList::const_iterator it;
	for (it = buffer_ifaces.begin(); it != buffer_ifaces.end(); ++it) {
		RecursionBufferId binding = (*it)->binding();
		// Mapgen-buffers are allways cached
		assert(binding & BufferCacheFlag);
		int idx = binding & BufferIndexMask;
		(*it)->bind(frag->buffers[idx]);
	}
}

void RecursiveMapgen::generateTerrain()
{
	v2s32 pos_min(node_min.X, node_min.Z);
	v2s32 pos_max(node_max.X, node_max.Z);
	v2s32 frag_min = getContainerPos32(v2s32(pos_min - grid_origin), frag_size);
	v2s32 frag_max = getContainerPos32(v2s32(pos_max - grid_origin), frag_size);

	for (s32 fx = frag_min.X; fx <= frag_max.X; ++fx)
	for (s32 fz = frag_min.Y; fz <= frag_max.Y; ++fz) {
		RecursionFragment* frag = getFragment(fx * frag_size, fz * frag_size);

		// Bind the fragment to the mapgen buffers
		BufferIfaceList::const_iterator it;
		for (it = buffer_ifaces.begin(); it != buffer_ifaces.end(); ++it) {
			RecursionBufferId binding = (*it)->binding();
			// Mapgen-buffers are allways cached
			assert(binding & BufferCacheFlag);
			int idx = binding & BufferIndexMask;
			(*it)->bind(frag->buffers[idx]);
		}

		int x_min =  fx      * frag_size     + grid_origin.X;
		int x_max = (fx + 1) * frag_size - 1 + grid_origin.X;
		int z_min =  fz      * frag_size     + grid_origin.Y;
		int z_max = (fz + 1) * frag_size - 1 + grid_origin.Y;
		patch_min = v3s16(std::max<s16>(x_min, node_min.X), node_min.Y, std::max<s16>(z_min, node_min.Z));
		patch_max = v3s16(std::min<s16>(x_max, node_max.X), node_max.Y, std::min<s16>(z_max, node_max.Z));

		generateTerrainPatch();
	}
}

RecursionBufferId RecursiveMapgen::addBuffer(BufferIface * iface) {
	int idx = cache_buffer_infos.size();
	if(idx > BufferIndexMask)
		throw BaseException("Too many buffers in use");
	RecursionBufferId id = BufferCacheFlag | idx;
	BufferInfo info;
	info.id = id;
	info.type = iface->type();
	info.initialized = false;
	info.consumed = false;
	info.splitted = false;
	info.border_nx = info.border_nz = info.border_px = info.border_pz = 0;
	cache_buffer_infos.push_back(info);
	iface->setBinding(id);
	buffer_ifaces.push_back(iface);
	return id;
}

RecursionBufferId RecursiveMapgen::addTemporaryBuffer(RecursionBufferType type, bool split)
{
	int idx = temp_buffer_infos.size();
	if(idx > BufferIndexMask)
		throw BaseException("Too many temporary buffers in use");
	RecursionBufferId id = idx;
	BufferInfo info;
	info.id = id;
	info.type = type;
	info.initialized = false;
	info.consumed = false;
	info.splitted = split;
	info.border_nx = info.border_nz = info.border_px = info.border_pz = 0;
	temp_buffer_infos.push_back(info);
	return id;
}

void RecursiveMapgen::addPass(RecursionPass * pass, std::vector<RecursionBufferId> binding) {
	const std::vector<BufferIface*>& buffers = pass->buffers();
	if (buffers.size() != binding.size())
		throw BaseException("Bindings don't fit to pass");

	for(unsigned int i = 0; i < buffers.size(); ++i) {
		BufferIface * buffer = buffers[i];
		bool reading = buffer->isInput();
		bool writing = buffer->isOutput();
		bool splitted = buffer->isSplitted();
		bool cached = binding[i] & BufferCacheFlag;
		int index = binding[i] & BufferIndexMask;
		if(cached) {
			if(reading && splitted && !cache_buffer_infos[index].initialized)
				throw BaseException("Pass trying to read highres data before it was generated.");
			if(writing && !splitted)
				throw BaseException("Pass trying to overwrite lowres data from previous iteration.");

			cache_buffer_infos[index].initialized |= writing;
			cache_buffer_infos[index].consumed |= reading && !splitted;
		} else {
			if (reading && !temp_buffer_infos[index].initialized)
				throw BaseException("Pass is consuming unitialized buffer");
			if (splitted != temp_buffer_infos[index].splitted)
				throw BaseException("Pass wants to index buffer in an incompatible way");
			temp_buffer_infos[index].initialized |= writing;
			temp_buffer_infos[index].consumed |= reading;
		}
		buffer->setBinding(binding[i]);
	}

	passes.push_back(pass);
}

inline static u32 nextPowerOfTwo(u32 x)
{
	x = x - 1;
	for(int i = 0; i < 5; ++i)
		x = x | (x >> (1 << i));
	return x + 1;
}

void RecursiveMapgen::finalize()
{
	// Check buffers for consistent use
	BufferInfoList::iterator biit;
	for (biit = cache_buffer_infos.begin(); biit != cache_buffer_infos.end(); ++biit) {
		if(!biit->initialized)
			throw BaseException("Buffer gets never written to");
		if(!biit->consumed)
			throw BaseException("Buffer is never read from");
	}
	for (biit = temp_buffer_infos.begin(); biit != temp_buffer_infos.end(); ++biit) {
		if(!biit->initialized)
			throw BaseException("Buffer gets never written to");
		if(!biit->consumed)
			throw BaseException("Buffer is never read from");
	}

	// Calculate minimal necessary border for passes and buffers.
	// This includes the whole range that would be scanned when
	// one fragment without border should be created in on go.
	int border_nx = 0;
	int border_px = 0;
	int border_nz = 0;
	int border_pz = 0;
	PassList::const_reverse_iterator rit;
	for (rit = passes.rbegin(); rit != passes.rend(); ++rit) {
		RecursionPass *pass = *rit;
		u8 nx = 0, px = 0, nz = 0, pz = 0;

		const std::vector<BufferIface*> &buffers = pass->buffers();
		std::vector<BufferIface*>::const_iterator bit;
		for (bit = buffers.begin(); bit != buffers.end(); ++bit) {
			BufferIface *iface = *bit;

			RecursionBufferId binding = iface->binding();
			bool cached = binding & BufferCacheFlag;
			int index = binding & BufferIndexMask;
			const BufferInfo &info = cached ? cache_buffer_infos[index]
			                                : temp_buffer_infos[index];

			if (iface->isOutput()) {
				if (iface->isSplitted()) {
					nx = std::max<u8>(nx, (info.border_nz + 1) / 2);
					px = std::max<u8>(px, (info.border_pz + 1) / 2);
					nz = std::max<u8>(nz, info.border_nx);
					pz = std::max<u8>(pz, info.border_px);
				} else {
					nx = std::max<u8>(nx, info.border_nx);
					px = std::max<u8>(px, info.border_px);
					nz = std::max<u8>(nz, info.border_nz);
					pz = std::max<u8>(pz, info.border_pz);
				}
			}
		}

		// Store nx,px,nz,pz for the pass as this will be the generation area
		pass->border_nx = nx;
		pass->border_px = px;
		pass->border_nz = nz;
		pass->border_pz = pz;

		// Grow the input buffers to have enough data for the pass
		for (bit = buffers.begin(); bit != buffers.end(); ++bit) {
			BufferIface *iface = *bit;

			RecursionBufferId binding = iface->binding();
			bool cached = binding & BufferCacheFlag;
			int index = binding & BufferIndexMask;
			BufferInfo &info = cached ? cache_buffer_infos[index]
			                          : temp_buffer_infos[index];

			if (iface->isInput()) {
				if (iface->isSplitted()) {
					info.border_nz = std::max<u8>(info.border_nz, (nx + iface->scanRangeNeg().X) * 2);
					info.border_pz = std::max<u8>(info.border_pz, (px + iface->scanRangePos().X) * 2);
					info.border_nx = std::max<u8>(info.border_nx, (nz + iface->scanRangeNeg().Y));
					info.border_px = std::max<u8>(info.border_px, (pz + iface->scanRangePos().Y));
				} else {
					info.border_nx = std::max<u8>(info.border_nx, nx + iface->scanRangeNeg().X);
					info.border_px = std::max<u8>(info.border_px, px + iface->scanRangePos().X);
					info.border_nz = std::max<u8>(info.border_nz, nz + iface->scanRangeNeg().Y);
					info.border_pz = std::max<u8>(info.border_pz, pz + iface->scanRangePos().Y);
				}

				border_nx = std::max<u8>(border_nx, nx + iface->scanRangeNeg().X);
				border_px = std::max<u8>(border_px, px + iface->scanRangePos().X);
				border_nz = std::max<u8>(border_nz, nz + iface->scanRangeNeg().Y);
				border_pz = std::max<u8>(border_pz, pz + iface->scanRangePos().Y);
			}
		}
	}

	// NOTE: JUST A TEST (It looks like my border calculation is somewhat wrong,
	//                    this increases it to compensate this)
	border_nx *= 2;
	border_nz *= 2;
	border_px *= 2;
	border_pz *= 2;

	// Decide size of fragments based on border-overhead

	u8 reserve_n = border_nx + border_nz;
	u8 reserve_p = border_px + border_pz;

	frag_size = nextPowerOfTwo((reserve_n + reserve_p) * 6);

	// Update passes and buffers to contain the reserve and
	// to also include the border necessary for further passes
	// until the next border-doubling by splitting.
	// After this buffers are sufficient to store the output
	// of the passes and contain the input for passes that
	// read from them.

	PassList::const_iterator pit;
	for (pit = passes.begin(); pit != passes.end(); ++pit) {
		RecursionPass *pass = *pit;

		// Grow the pass to contain the reserve and the border for the next pass
		pass->border_nx += reserve_n;
		pass->border_px += reserve_p;
		pass->border_nz += reserve_n + border_nx;
		pass->border_pz += reserve_p + border_px;

		// Grow the output buffers so that they can hold processing results
		const std::vector<BufferIface*> &buffers = pass->buffers();
		std::vector<BufferIface*>::const_iterator bit;
		for (bit = buffers.begin(); bit != buffers.end(); ++bit) {
			BufferIface *iface = *bit;

			RecursionBufferId binding = iface->binding();
			bool cached = binding & BufferCacheFlag;
			int index = binding & BufferIndexMask;
			BufferInfo &info = cached ? cache_buffer_infos[index]
			                          : temp_buffer_infos[index];

			if (iface->isOutput()) {
				if (iface->isSplitted()) {
					info.border_nz = std::max<u8>(info.border_nz, pass->border_nx * 2);
					info.border_pz = std::max<u8>(info.border_pz, pass->border_px * 2);
					info.border_nx = std::max<u8>(info.border_nx, pass->border_nz);
					info.border_px = std::max<u8>(info.border_px, pass->border_pz);
				} else {
					info.border_nx = std::max<u8>(info.border_nx, pass->border_nx);
					info.border_px = std::max<u8>(info.border_px, pass->border_px);
					info.border_nz = std::max<u8>(info.border_nz, pass->border_nz);
					info.border_pz = std::max<u8>(info.border_pz, pass->border_pz);
				}
			}
		}
	}

	// Set size of all buffers and allocate temporary ones
	for (biit = cache_buffer_infos.begin(); biit != cache_buffer_infos.end(); ++biit) {
		biit->full_width = biit->border_nx + frag_size + biit->border_px;
		biit->full_height = biit->border_nz + frag_size + biit->border_pz;
		biit->buffer_size = biit->full_width * biit->full_height;
	}
	for (biit = temp_buffer_infos.begin(); biit != temp_buffer_infos.end(); ++biit) {
		// TODO Temporary buffers that aren't splitted could use frag_size/2 for width
		biit->full_width = biit->border_nx + frag_size + biit->border_px;
		biit->full_height = biit->border_nz + frag_size + biit->border_pz;
		biit->buffer_size = biit->full_width * biit->full_height;
		temp_buffers.push_back(createBuffer(*biit));
	}

	// Store border size for root fragments
	// TODO I'm not absolutely sure if this is correct
	this->border_nx = reserve_n;
	this->border_nz = reserve_n + border_nx;
	this->border_px = reserve_p;
	this->border_pz = reserve_p + border_px;
}

RecursionFragment *RecursiveMapgen::getFragment(s32 x, s32 z, int level, int split)
{
	unsigned int cache_index = level * 2 + split;
	unsigned int pos_parent_x = z >> 1;
	unsigned int pos_parent_z = x;

	if(cache.size() <= cache_index)
		cache.resize(cache_index + 1);

	// Try to take the fragment from the cache
	FragmentLevelCache::const_iterator it = cache[cache_index].find(v2s32(x, z));
	if(it != cache[cache_index].end())
		return it->second;

	if(level > root_level || (level == root_level && split != 0))
		throw BaseException("Invalid level/split combination");

	if(level == root_level) {
		RecursionFragment *root = createFragment(x, z, level, split);

		// Bind the fragment to the mapgen buffers
		BufferIfaceList::const_iterator it;
		for (it = buffer_ifaces.begin(); it != buffer_ifaces.end(); ++it) {
			RecursionBufferId binding = (*it)->binding();
			// Mapgen-buffers are allways cached
			assert(binding & BufferCacheFlag);
			int idx = binding & BufferIndexMask;
			(*it)->bind(root->buffers[idx]);
		}

		// TODO Decide part that has to be filled in in the root fragment

		patch_min.X = root->pos_x - border_nx;
		patch_max.X = root->pos_x + frag_size / 2 + border_px - 1;
		patch_min.Z = root->pos_z - border_nz;
		patch_max.Z = root->pos_z + frag_size + border_pz - 1;

		generateRootFragment();

		return root;
	}

	RecursionFragment *parent;

	if(split == 0) {
		parent = getFragment(pos_parent_x / frag_size * frag_size, pos_parent_z, level, 1);
	} else {
		parent = getFragment(pos_parent_x / frag_size * frag_size, pos_parent_z, level + 1, 0);
	}

	RecursionFragment *frag = createFragment(x, z, level, split);

	PassList::const_iterator it2;
	for (it2 = passes.begin(); it2 != passes.end(); ++it2) {
		RecursionPass *pass = *it2;
		const std::vector<BufferIface*>& buffers = pass->buffers();

		std::vector<BufferIface*>::const_iterator it3;
		for (it3 = buffers.begin(); it3 != buffers.end(); ++it3) {
			BufferIface *buffer = (*it3);
			RecursionBufferId binding = buffer->binding();
			bool cached = binding & BufferCacheFlag;
			int index = binding & BufferIndexMask;
			if (!cached) {
				positionBuffer(frag->pos_x, frag->pos_z,
							   &temp_buffers[index], temp_buffer_infos[index]);
				buffer->bind(temp_buffers[index]);
			} else if(buffer->isSplitted()) {
				buffer->bind(frag->buffers[index]);
			} else {
				buffer->bind(parent->buffers[index]);
			}
		}

		// NOTE Splitting is done in the parent-coordinate system

		int min_x = pos_parent_x - pass->border_nx;
		int max_x = pos_parent_x + frag_size / 2 + pass->border_px - 1;
		int min_z = pos_parent_z - pass->border_nz;
		int max_z = pos_parent_z + frag_size + pass->border_pz - 1;

		pass->doSplit(min_x, max_x, min_z, max_z, level);
	}

	return frag;
}

RecursionFragment *RecursiveMapgen::createFragment(s32 x, s32 z, int level, int split)
{
	RecursionFragment *fragment = new RecursionFragment();
	fragment->level = level;
	fragment->split = split;
	fragment->pos_x = x;
	fragment->pos_z = z;

	BufferInfoList::const_iterator it;
	for (it = cache_buffer_infos.begin(); it != cache_buffer_infos.end(); ++it) {
		RecursionBuffer buffer = createBuffer(*it);
		positionBuffer(x, z, &buffer, *it);
		fragment->buffers.push_back(buffer);
	}

	int cache_index = level * 2 + split;
	cache[cache_index][v2s32(x, z)] = fragment;
	return fragment;
}

RecursionBuffer RecursiveMapgen::createBuffer(const BufferInfo & info)
{
	RecursionBuffer buffer;
	switch(info.type) {
	case RBT_Int:
		buffer.data = new int[info.buffer_size];
		break;
	case RBT_Float:
		buffer.data = new float[info.buffer_size];
		break;
	case RBT_Double:
		buffer.data = new double[info.buffer_size];
		break;
	case RBT_Vector2D:
		buffer.data = new core::vector2d<float>[info.buffer_size];
		break;
	}
	return buffer;
}

void RecursiveMapgen::positionBuffer(s32 x, s32 z, RecursionBuffer * buffer, const BufferInfo & info)
{
	buffer->offset_x = info.border_nx - x;
	if(info.splitted)
		buffer->offset_x /= 2;
	buffer->offset_z = info.border_nz - z;
	buffer->delta_z = info.full_width;
	buffer->size = info.buffer_size;
}

/* ===== DEMO AREA ============ */

NoisePass::NoisePass(int seed)
    : seed(seed)
    , input(1, 1) // This pass scans one cell in -X and +X directions
	, output()
{
	addBuffer(&input);
	addBuffer(&output);
}

void NoisePass::doSplit(int min_x, int max_x, int min_z, int max_z, int level)
{

	float amplitude = 0.2 * pow(2.1, level);
	for (int z = min_z; z < max_z; ++z)
	for (int x = min_x; x < max_x; ++x) {
		// Get value of cell
		float self_value = input.get(x, z);
		// Get the interpolated child values
		float left_value = (input.get(x-1, z) + self_value * 3) / 4;
		float right_value = (input.get(x+1, z) + self_value * 3) / 4;
		// Generate some additional noise to add in this pass
		float left_noise = noise3d(x * 2, z, level, seed);
		float right_noise = noise3d(x * 2 + 1, z, level, seed);
		// Combine the interpolated value with the noise to get the output
		output.setLeft(x, z, left_value + amplitude * left_noise);
		output.setRight(x, z, right_value + amplitude * right_noise);
	}
}



Mapgen_Test::Mapgen_Test(int mapgenid, MapgenParams * params, EmergeManager * emerge)
    : RecursiveMapgen(mapgenid, params, emerge)
    , noise_buffer()
    , noise_pass(params->seed + 42)
{
	RecursionBufferId noise_buffer_id = addBuffer(&noise_buffer);

	std::vector<RecursionBufferId> noise_binding;
	// The noise pass takes the noise from the previous iteration
	noise_binding.push_back(noise_buffer_id);
	// The noise pass write the noise for this iteration
	noise_binding.push_back(noise_buffer_id);
	addPass(&noise_pass, noise_binding);

	finalize();

	c_stone = ndef->getId("mapgen_stone");
}

MapgenType Mapgen_Test::getType() const
{
	return MAPGEN_RR_DEMO;
}

void Mapgen_Test::makeChunk(BlockMakeData * data)
{
	this->generating = true;
	this->vm   = data->vmanip;
	this->ndef = data->nodedef;

	v3s16 blockpos_min = data->blockpos_min;
	v3s16 blockpos_max = data->blockpos_max;
	node_min = blockpos_min * MAP_BLOCKSIZE;
	node_max = (blockpos_max + v3s16(1, 1, 1)) * MAP_BLOCKSIZE - v3s16(1, 1, 1);

	// Generate the terrain by recursively refining and finally
	// convert it to nodes by invoking generateTerrainPatch()
	generateTerrain();

	// TODO Add everything else like biomes, decoration and lighting etc.

	// Create heightmap
	updateHeightmap(node_min, node_max);

	// Add top and bottom side of water to transforming_liquid queue
	updateLiquid(&data->transforming_liquid, node_min, node_max);

	// Set lighting
	if (flags & MG_LIGHT)
		setLighting(LIGHT_SUN, node_min, node_max);

	this->generating = false;
}

int Mapgen_Test::getGroundLevelAtPoint(v2s16 p)
{
	generateAndBindPosition(p.X, p.Y);
	return noise_buffer.get(p.X - grid_origin.X, p.Y - grid_origin.Y);
}

int Mapgen_Test::getSpawnLevelAtPoint(v2s16 p)
{
	generateAndBindPosition(p.X, p.Y);
	return int(noise_buffer.get(p.X - grid_origin.X, p.Y - grid_origin.Y)) + 1;
}

void Mapgen_Test::generateTerrainPatch()
{
	// When generateTerrainPatch is called the recursive refinement
	// is already completed and the resulting data is bound to the
	// registerd buffers (in this case the noise_buffer)

	MapNode n_air(CONTENT_AIR);
	MapNode n_stone(c_stone);

	v3s16 em = vm->m_area.getExtent();

	assert(patch_min.X < patch_max.X);
	assert(patch_min.Z < patch_max.Z);

	for (s16 z = patch_min.Z; z <= patch_max.Z; ++z)
	for (s16 x = patch_min.X; x <= patch_max.X; ++x) {
		float stone_level = noise_buffer.get(x, z);

		u32 vi = vm->m_area.index(x, patch_min.Y - 1, z);
		for (s16 y = patch_min.Y - 1; y <= patch_max.Y + 1; ++y) {
			if (vm->m_data[vi].getContent() == CONTENT_IGNORE) {
				if (y <= stone_level) {
					vm->m_data[vi] = n_stone;
				} else {
					vm->m_data[vi] = n_air;
				}
				vm->m_area.add_y(em, vi, 1);
			}
		}
	}
}

void Mapgen_Test::generateRootFragment()
{
	// NOTE patch_min/patch_max coordinates don't match to node coordinates
	for (s16 z = patch_min.Z; z <= patch_max.Z; ++z)
	for (s16 x = patch_min.X; x <= patch_max.X; ++x) {
		noise_buffer.set(x, z, 0);
	}
}
