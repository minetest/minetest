#pragma once
#include "util/numeric.h"
#include "nodedef.h"
#include <IMeshManipulator.h>

// Mesh options
static const u8 MO_MASK_STYLE          = 0x07;
static const u8 MO_BIT_RANDOM_OFFSET   = 0x08;
static const u8 MO_BIT_SCALE_SQRT2     = 0x10;
static const u8 MO_BIT_RANDOM_OFFSET_Y = 0x20;
enum PlantlikeStyle {
	PLANT_STYLE_CROSS,
	PLANT_STYLE_CROSS2,
	PLANT_STYLE_STAR,
	PLANT_STYLE_HASH,
	PLANT_STYLE_HASH2,
};

struct LightFrame
{
	f32 lightsA[8];
	f32 lightsB[8];
	u8 light_source;
};

class MapblockMeshGenerator
{
public:
	MeshMakeData *data;
	MeshCollector *collector;

	INodeDefManager *nodedef;
	scene::ISceneManager *smgr;
	scene::IMeshManipulator *meshmanip;

// options
	bool enable_mesh_cache;

// current node
	v3s16 blockpos_nodes;
	v3s16 p;
	v3f origin;
	MapNode n;
	const ContentFeatures *f;
	u16 light;
	LightFrame frame;
	video::SColor color;
	TileSpec tile;
	float scale;

// cuboid drawing!
	void drawCuboid(const aabb3f &box, TileSpec *tiles, int tilecount,
		const video::SColor *colors, const f32 *txc);
	void drawSmoothLightedCuboid(const aabb3f &box, TileSpec *tiles, int tilecount,
		const u16 *lights , const f32 *txc);
	void drawCuboid(const aabb3f &box, TileSpec *tiles, int tilecount, const f32 *txc);
	void drawAutoLightedCuboid(aabb3f box);
	void drawAutoLightedCuboidEx(aabb3f box, const f32 *txc);

// liquid-specific
	bool top_is_same_liquid;
	TileSpec tile_liquid;
	TileSpec tile_liquid_top;
	content_t c_flowing;
	content_t c_source;
	video::SColor color_liquid_top;
	struct NeighborData {
		f32 level;
		content_t content;
		bool is_same_liquid;
		bool top_is_same_liquid;
	};
	NeighborData liquid_neighbors[3][3];
	f32 corner_levels[2][2];

	void prepareLiquidNodeDrawing(bool flowing);
	void getLiquidNeighborhood(bool flowing);
	void resetCornerLevels();
	void calculateCornerLevels();
	f32 getCornerLevel(int i, int k);
	void drawLiquidSides(bool flowing);
	void drawLiquidTop(bool flowing);

// raillike-specific
	// name of the group that enables connecting to raillike nodes of different kind
	static const std::string raillike_groupname;
	int raillike_group;
	bool isSameRail(v3s16 dir);

// plantlike-specific
	PlantlikeStyle draw_style;
	v3f offset;
	int rotate_degree;
	bool random_offset_Y;
	int face_num;

	void drawPlantlikeQuad(float rotation, float quad_offset = 0,
		bool offset_top_only = false);

// firelike-specific
	void drawFirelikeQuad(float rotation, float opening_angle,
		float offset_h, float offset_v = 0.0);

// drawtypes
	void drawLiquidNode(bool flowing);
	void drawGlasslikeNode();
	void drawGlasslikeFramedNode();
	void drawAllfacesNode();
	void drawTorchlikeNode();
	void drawSignlikeNode();
	void drawPlantlikeNode();
	void drawFirelikeNode();
	void drawFencelikeNode();
	void drawRaillikeNode();
	void drawNodeboxNode();
	void drawMeshNode();

	void drawNode();

public:
	MapblockMeshGenerator(MeshMakeData *input, MeshCollector *output);
	void generate();
};

/// Direction in the 6D format. See g_6dirs for direction vectors.
/// Here P means Positive, N stands for Negative.
enum Direction6D {
// 0
	D6D_ZP,
	D6D_YP,
	D6D_XP,
	D6D_ZN,
	D6D_YN,
	D6D_XN,
// 6
	D6D_XN_YP,
	D6D_XP_YP,
	D6D_YP_ZP,
	D6D_YP_ZN,
	D6D_XN_ZP,
	D6D_XP_ZP,
	D6D_XN_ZN,
	D6D_XP_ZN,
	D6D_XN_YN,
	D6D_XP_YN,
	D6D_YN_ZP,
	D6D_YN_ZN,
// 18
	D6D_XN_YP_ZP,
	D6D_XP_YP_ZP,
	D6D_XN_YP_ZN,
	D6D_XP_YP_ZN,
	D6D_XN_YN_ZP,
	D6D_XP_YN_ZP,
	D6D_XN_YN_ZN,
	D6D_XP_YN_ZN,
// 26
	D6D,

// aliases
	D6D_BACK   = D6D_ZP,
	D6D_TOP    = D6D_YP,
	D6D_RIGHT  = D6D_XP,
	D6D_FRONT  = D6D_ZN,
	D6D_BOTTOM = D6D_YN,
	D6D_LEFT   = D6D_XN,
};

/// Direction in the wallmounted format.
/// P is Positive, N is Negative.
enum DirectionWallmounted {
	DWM_YP,
	DWM_YN,
	DWM_XP,
	DWM_XN,
	DWM_ZP,
	DWM_ZN,
};
