#pragma once

#include "irrTypes.h"


struct ClientDynamicInfo
{
	v2u32 render_target_size;
	f32 gui_scaling;
	f32 hud_scaling;

	bool equal(const ClientDynamicInfo &other) const {
		return render_target_size == other.render_target_size &&
				abs(gui_scaling - other.gui_scaling) < 0.001f &&
				abs(hud_scaling - other.hud_scaling) < 0.001f;
	}
};
