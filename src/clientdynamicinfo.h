#pragma once

#include "irrTypes.h"


struct ClientDynamicInfo
{
	v2u32 render_target_size;
	f32 real_gui_scaling;
	f32 real_hud_scaling;

	bool equal(const ClientDynamicInfo &other) const {
		return render_target_size == other.render_target_size &&
				abs(real_gui_scaling - other.real_gui_scaling) < 0.001f &&
				abs(real_hud_scaling - other.real_hud_scaling) < 0.001f;
	}
};
