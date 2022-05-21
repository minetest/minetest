#pragma once

#include "irrTypes.h"


struct ClientDynamicInfo
{
	v2u32 screen_size;
	f32 dpi;
	f32 gui_scaling;
	f32 hud_scaling;

	bool equal(const ClientDynamicInfo &other) const {
		return screen_size == other.screen_size &&
				abs(dpi - other.dpi) < 0.001f &&
				abs(gui_scaling - other.gui_scaling) < 0.001f &&
				abs(hud_scaling - other.hud_scaling) < 0.001f;
	}
};
