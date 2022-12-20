#pragma once

#include "irrTypes.h"


struct ClientDynamicInfo
{
	v2u32 render_target_size;
	f32 real_gui_scaling;
	f32 real_hud_scaling;
	v2f32 max_fs_size;

	bool equal(const ClientDynamicInfo &other) const {
		return render_target_size == other.render_target_size &&
				abs(real_gui_scaling - other.real_gui_scaling) < 0.001f &&
				abs(real_hud_scaling - other.real_hud_scaling) < 0.001f;
	}

	static v2f32 calculateMaxFSSize(v2u32 render_target_size, f32 real_gui_scaling) {
		float slot_size = 0.5555f * 96.f * real_gui_scaling;
		return v2f32(render_target_size.X, render_target_size.Y) / slot_size;
	}
};
