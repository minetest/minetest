#include "materials.h"

#define MATERIAL_PROPERTIES_COUNT 256

// These correspond to the CONTENT_* constants
MaterialProperties g_material_properties[MATERIAL_PROPERTIES_COUNT];

bool g_material_properties_initialized = false;

void setStoneLikeDiggingProperties(u8 material, float toughness)
{
	g_material_properties[material].setDiggingProperties("",
			DiggingProperties(true, 15.0*toughness, 0));
	
	g_material_properties[material].setDiggingProperties("WPick",
			DiggingProperties(true, 1.5*toughness, 65535./30.*toughness));
	g_material_properties[material].setDiggingProperties("STPick",
			DiggingProperties(true, 0.7*toughness, 65535./100.*toughness));

	/*g_material_properties[material].setDiggingProperties("MesePick",
			DiggingProperties(true, 0.0*toughness, 65535./20.*toughness));*/
}

void initializeMaterialProperties()
{
	/*
		Now, the g_material_properties array is already initialized
		by the constructors to such that no digging is possible.

		Add some digging properties to them.
	*/
	
	setStoneLikeDiggingProperties(CONTENT_STONE, 1.0);

	g_material_properties[CONTENT_GRASS].setDiggingProperties("",
			DiggingProperties(true, 0.5, 0));

	g_material_properties[CONTENT_TORCH].setDiggingProperties("",
			DiggingProperties(true, 0.0, 0));

	g_material_properties[CONTENT_TREE].setDiggingProperties("",
			DiggingProperties(true, 1.5, 0));

	g_material_properties[CONTENT_LEAVES].setDiggingProperties("",
			DiggingProperties(true, 0.5, 0));

	g_material_properties[CONTENT_GRASS_FOOTSTEPS].setDiggingProperties("",
			DiggingProperties(true, 0.5, 0));

	setStoneLikeDiggingProperties(CONTENT_MESE, 0.5);

	g_material_properties[CONTENT_MUD].setDiggingProperties("",
			DiggingProperties(true, 0.5, 0));

	setStoneLikeDiggingProperties(CONTENT_COALSTONE, 1.5);

	g_material_properties[CONTENT_WOOD].setDiggingProperties("",
			DiggingProperties(true, 1.0, 0));
	
	/*
		Add MesePick to everything
	*/
	for(u16 i=0; i<MATERIAL_PROPERTIES_COUNT; i++)
	{
		g_material_properties[i].setDiggingProperties("MesePick",
				DiggingProperties(true, 0.0, 65535./1337));
	}

	g_material_properties_initialized = true;
}

MaterialProperties * getMaterialProperties(u8 material)
{
	assert(g_material_properties_initialized);
	return &g_material_properties[material];
}

DiggingProperties getDiggingProperties(u8 material, const std::string &tool)
{
	MaterialProperties *mprop = getMaterialProperties(material);
	if(mprop == NULL)
		// Not diggable
		return DiggingProperties();
	
	return mprop->getDiggingProperties(tool);
}

