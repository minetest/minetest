-- Luanti
-- SPDX-License-Identifier: LGPL-2.1-or-later
-- Copyright (C) 2023 v-rob, Vincent Robinson <robinsonvincent89@gmail.com>

local prelude_theme = ui.Style {
	ui.Style "root" {
		-- The root window should be centered on the screen, but have no size
		-- by default, since the size should be user-set.
		rel_pos = {1/2, 1/2},
		rel_anchor = {1/2, 1/2},
		rel_size = {0, 0},

		-- Normally, we don't show the backdrop unless we have user focus.
		ui.Style "@backdrop" {
			visible = false,
		},
		ui.Style "@backdrop$focused" {
			visible = true,
		}
	},
	ui.Style "image" {
		icon_scale = 0,
	},
	ui.Style "check, switch, radio" {
		icon_place = "left",
	},
}

function ui.get_prelude_theme()
	return prelude_theme
end

local default_theme = prelude_theme

function ui.get_default_theme()
	return default_theme
end

function ui.set_default_theme(theme)
	default_theme = theme
end
