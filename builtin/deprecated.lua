-- Minetest: builtin/deprecated.lua

--
-- Default material types
--
function digprop_err()
	minetest.log("info", debug.traceback())
	minetest.log("info", "WARNING: The minetest.digprop_* functions are obsolete and need to be replaced by item groups.")
end

minetest.digprop_constanttime = digprop_err
minetest.digprop_stonelike = digprop_err
minetest.digprop_dirtlike = digprop_err
minetest.digprop_gravellike = digprop_err
minetest.digprop_woodlike = digprop_err
minetest.digprop_leaveslike = digprop_err
minetest.digprop_glasslike = digprop_err


