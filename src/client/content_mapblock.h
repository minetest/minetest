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

#pragma once

#include "nodedef.h"
#include <IMeshManipulator.h>
#include <array>

struct MeshMakeData;
struct MeshCollector;

enum class QuadDiagonal {
	Diag02,
	Diag13,
};

struct LightPair {
	u8 lightDay;
	u8 lightNight;

	LightPair() = default;
	explicit LightPair(u16 value) : lightDay(value & 0xff), lightNight(value >> 8) {}
	LightPair(u8 valueA, u8 valueB) : lightDay(valueA), lightNight(valueB) {}
	LightPair(float valueA, float valueB) :
		lightDay(core::clamp(core::round32(valueA), 0, 255)),
		lightNight(core::clamp(core::round32(valueB), 0, 255)) {}
	operator u16() const { return lightDay | lightNight << 8; }

	static int lightDiff(LightPair a, LightPair b);
	static QuadDiagonal quadDiagonalForFace(LightPair final_lights[4]);
};

struct LightInfo {
	float light_day;
	float light_night;
	float light_boosted;

	LightPair getPair(float sunlight_boost = 0.0) const
	{
		return LightPair(
			(1 - sunlight_boost) * light_day
			+ sunlight_boost * light_boosted,
			light_night);
	}
};

struct LightFrame {
	f32 lightsDay[8];
	f32 lightsNight[8];
	bool sunlight[8];
};

class MapblockMeshGenerator
{
public:
	MapblockMeshGenerator(MeshMakeData *input, MeshCollector *output,
			scene::IMeshManipulator *mm);
	void generate();
	void renderSingle(content_t node, u8 param2 = 0x00);

private:
	MeshMakeData *const data;
	MeshCollector *const collector;

	const NodeDefManager *const nodedef;
	scene::IMeshManipulator *const meshmanip;

	const v3s16 blockpos_nodes;

// options
	const bool enable_mesh_cache;

// current node
	struct {
		v3s16 p;
		v3f origin;
		MapNode n;
		const ContentFeatures *f;
		LightPair light;
		LightFrame frame;
		video::SColor color;
		TileSpec tile;
		f32 scale;
	} cur_node;

// lighting
	void getSmoothLightFrame();
	LightInfo blendLight(const v3f &vertex_pos);
	video::SColor blendLightColor(const v3f &vertex_pos);
	video::SColor blendLightColor(const v3f &vertex_pos, const v3f &vertex_normal);

	void useTile(int index = 0, u8 set_flags = MATERIAL_FLAG_CRACK_OVERLAY,
		u8 reset_flags = 0, bool special = false);
	void getTile(int index, TileSpec *tile);
	void getTile(v3s16 direction, TileSpec *tile);
	void getSpecialTile(int index, TileSpec *tile, bool apply_crack = false);

// face drawing
	void drawQuad(v3f *vertices, const v3s16 &normal = v3s16(0, 0, 0),
		float vertical_tiling = 1.0);

// cuboid drawing!
	template <typename Fn>
	void drawCuboid(const aabb3f &box, TileSpec *tiles, int tilecount, const f32 *txc, u8 mask, Fn &&face_lighter);
	void generateCuboidTextureCoords(aabb3f const &box, f32 *coords);
	void drawAutoLightedCuboid(aabb3f box, f32 const *txc = nullptr, TileSpec *tiles = nullptr, int tile_count = 0, u8 mask = 0);
	u8 getNodeBoxMask(aabb3f box, u8 solid_neighbors, u8 sametype_neighbors) const;

// solid-specific
public:
	static bool isSolidFaceDrawn(content_t n1, const ContentFeatures &f1,
		content_t n2, const NodeDefManager *nodedef,
		bool *backface_culling_out = nullptr);

// liquid-specific
	struct LiquidData {
		struct NeighborData {
			f32 level; // in range [-0.5, 0.5]
			content_t content;
			bool is_same_liquid;
			bool top_is_same_liquid;
		};

		bool top_is_same_liquid;
		bool draw_bottom;
		TileSpec tile;
		TileSpec tile_top;
		content_t c_flowing;
		content_t c_source;
		video::SColor color_top;
		std::array<std::array<NeighborData, 3>, 3> neighbors;
		std::array<std::array<f32, 2>, 2> corner_levels;
	};
	LiquidData cur_liquid;
	bool smooth_liquids = false;

	static std::array<std::array<LiquidData::NeighborData, 3>, 3>
	getLiquidNeighborsData(content_t c_source, content_t c_flowing, u8 liquid_range,
			const std::function<MapNode(v3s16)> &get_node_rel);

	// the arrays are indexed Z-first, i.e. neighbors[z][x], ret[z][x]
	static std::array<std::array<f32, 2>, 2>
	calculateLiquidCornerLevels(content_t c_source, content_t c_flowing,
			const std::array<std::array<LiquidData::NeighborData, 3>, 3> &neighbors);

private:
	template <typename F>
	static std::array<std::array<LiquidData::NeighborData, 3>, 3>
	getLiquidNeighborsDataRaw(content_t c_source, content_t c_flowing,
			u8 liquid_range, F &&get_node_rel);

	static f32 calculateLiquidCornerLevel(content_t c_source, content_t c_flowing,
			const std::array<std::reference_wrapper<const LiquidData::NeighborData>, 4> &neighbors);

	void prepareLiquidNodeDrawing();
	void getLiquidNeighborhood();
	void calculateCornerLevels();
	void drawLiquidSides();
	void drawLiquidTop();
	void drawLiquidBottom();

// raillike-specific
	// name of the group that enables connecting to raillike nodes of different kind
	static const std::string raillike_groupname;
	struct RaillikeData {
		int raillike_group;
	};
	RaillikeData cur_rail;
	bool isSameRail(v3s16 dir);

// plantlike-specific
	struct PlantlikeData {
		PlantlikeStyle draw_style;
		v3f offset;
		float rotate_degree;
		bool random_offset_Y;
		int face_num;
		float plant_height;
	};
	PlantlikeData cur_plant;

	void drawPlantlikeQuad(float rotation, float quad_offset = 0,
		bool offset_top_only = false);
	void drawPlantlike(bool is_rooted = false);

// firelike-specific
	void drawFirelikeQuad(float rotation, float opening_angle,
		float offset_h, float offset_v = 0.0);

// drawtypes
	void drawSolidNode();
	void drawLiquidNode();
	void drawGlasslikeNode();
	void drawGlasslikeFramedNode();
	void drawAllfacesNode();
	void drawTorchlikeNode();
	void drawSignlikeNode();
	void drawPlantlikeNode();
	void drawPlantlikeRootedNode();
	void drawFirelikeNode();
	void drawFencelikeNode();
	void drawRaillikeNode();
	void drawNodeboxNode();
	void drawMeshNode();

// common
	void errorUnknownDrawtype();
	void drawNode();
};
