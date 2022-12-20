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

	static v2f32 calculateMaxFSSize(v2u32 render_target_size) {
		f32 factor =
#ifdef HAVE_TOUCHSCREENGUI
				10;
#else
				15;
#endif
		f32 ratio = (f32)render_target_size.X / (f32)render_target_size.Y;
		if (ratio < 1)
			return { factor, factor / ratio };
		else
			return { factor * ratio, factor };
	}
};
