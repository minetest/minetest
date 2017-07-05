#include "hud.h"


const struct EnumString es_HudElementType[] =
{
	{HUD_ELEM_IMAGE,     "image"},
	{HUD_ELEM_TEXT,      "text"},
	{HUD_ELEM_STATBAR,   "statbar"},
	{HUD_ELEM_INVENTORY, "inventory"},
	{HUD_ELEM_WAYPOINT,  "waypoint"},
	{0, NULL},
};

const struct EnumString es_HudElementStat[] =
{
	{HUD_STAT_POS,    "position"},
	{HUD_STAT_POS,    "pos"}, /* Deprecated, only for compatibility's sake */
	{HUD_STAT_NAME,   "name"},
	{HUD_STAT_SCALE,  "scale"},
	{HUD_STAT_TEXT,   "text"},
	{HUD_STAT_NUMBER, "number"},
	{HUD_STAT_ITEM,   "item"},
	{HUD_STAT_DIR,    "direction"},
	{HUD_STAT_ALIGN,  "alignment"},
	{HUD_STAT_OFFSET, "offset"},
	{HUD_STAT_WORLD_POS, "world_pos"},
	{0, NULL},
};

const struct EnumString es_HudBuiltinElement[] =
{
	{HUD_FLAG_HOTBAR_VISIBLE,    "hotbar"},
	{HUD_FLAG_HEALTHBAR_VISIBLE, "healthbar"},
	{HUD_FLAG_CROSSHAIR_VISIBLE, "crosshair"},
	{HUD_FLAG_WIELDITEM_VISIBLE, "wielditem"},
	{HUD_FLAG_BREATHBAR_VISIBLE, "breathbar"},
	{HUD_FLAG_MINIMAP_VISIBLE,   "minimap"},
	{0, NULL},
};
