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
	f32 getCornerLevel(u32 i, u32 k);
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
