#include "nodedef.h"

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

// kate: space-indent off; mixedindent on; indent-mode cstyle;
