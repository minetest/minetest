
unused_args = false

read_globals = {
	"ItemStack",
	"INIT",
	"DIR_DELIM",
	"dump", "dump2",
	"fgettext", "fgettext_ne",
	"vector",
	"nodeupdate", "nodeupdate_single",
	"VoxelArea",
	"profiler",
}

globals = {
	"core",
}

include_files = {
	"builtin/common",
	"builtin/game",
}

ignore = {
	--[[ whitespace
	"611",		-- A line consists of nothing but whitespace.
	"612",		-- A line contains trailing whitespace.
	--]]
	-- [[ shadowing
	"421",		-- Shadowing a local variable.
	"422",		-- Shadowing an argument.
	"423",		-- Shadowing a loop variable.
	"431",		-- Shadowing an upvalue.
	"432",		-- Shadowing an upvalue argument.
	"433",		-- Shadowing an upvalue loop variable.
	--]]
}

files["builtin/common/misc_helpers.lua"] = {
	globals = {
		"dump", "dump2", "table", "math", "string",
		"fgettext", "fgettext_ne", "basic_dump", "game", -- ???
		"file_exists", "get_last_folder", "cleanup_path", -- ???
	},
}

files["builtin/common/vector.lua"] = {
	globals = { "vector" },
}

files["builtin/game/falling.lua"] = {
	globals = { "nodeupdate", "nodeupdate_single" },
}

files["builtin/game/voxelarea.lua"] = {
	globals = { "VoxelArea" },
}

files["builtin/game/init.lua"] = {
	globals = { "profiler" },
}

files["builtin/common/filterlist.lua"] = {
	globals = {
		"filterlist",
		"compare_worlds", "sort_worlds_alphabetic", "sort_mod_list", -- ???
	},
}
