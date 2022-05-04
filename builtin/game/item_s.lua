-- Minetest: builtin/item_s.lua
-- The distinction of what goes here is a bit tricky, basically it's everything
-- that does not (directly or indirectly) need access to ServerEnvironment,
-- Server or writable access to IGameDef on the engine side.
-- (The '_s' stands for standalone.)

--
-- Item definition helpers
--

function core.inventorycube(img1, img2, img3)
	img2 = img2 or img1
	img3 = img3 or img1
	return "[inventorycube"
			.. "{" .. img1:gsub("%^", "&")
			.. "{" .. img2:gsub("%^", "&")
			.. "{" .. img3:gsub("%^", "&")
end

function core.dir_to_facedir(dir, is6d)
	--account for y if requested
	if is6d and math.abs(dir.y) > math.abs(dir.x) and math.abs(dir.y) > math.abs(dir.z) then

		--from above
		if dir.y < 0 then
			if math.abs(dir.x) > math.abs(dir.z) then
				if dir.x < 0 then
					return 19
				else
					return 13
				end
			else
				if dir.z < 0 then
					return 10
				else
					return 4
				end
			end

		--from below
		else
			if math.abs(dir.x) > math.abs(dir.z) then
				if dir.x < 0 then
					return 15
				else
					return 17
				end
			else
				if dir.z < 0 then
					return 6
				else
					return 8
				end
			end
		end

	--otherwise, place horizontally
	elseif math.abs(dir.x) > math.abs(dir.z) then
		if dir.x < 0 then
			return 3
		else
			return 1
		end
	else
		if dir.z < 0 then
			return 2
		else
			return 0
		end
	end
end

-- Table of possible dirs
local facedir_to_dir = {
	vector.new( 0,  0,  1),
	vector.new( 1,  0,  0),
	vector.new( 0,  0, -1),
	vector.new(-1,  0,  0),
	vector.new( 0, -1,  0),
	vector.new( 0,  1,  0),
}
-- Mapping from facedir value to index in facedir_to_dir.
local facedir_to_dir_map = {
	[0]=1, 2, 3, 4,
	5, 2, 6, 4,
	6, 2, 5, 4,
	1, 5, 3, 6,
	1, 6, 3, 5,
	1, 4, 3, 2,
}
function core.facedir_to_dir(facedir)
	return facedir_to_dir[facedir_to_dir_map[facedir % 32]]
end

function core.dir_to_wallmounted(dir)
	if math.abs(dir.y) > math.max(math.abs(dir.x), math.abs(dir.z)) then
		if dir.y < 0 then
			return 1
		else
			return 0
		end
	elseif math.abs(dir.x) > math.abs(dir.z) then
		if dir.x < 0 then
			return 3
		else
			return 2
		end
	else
		if dir.z < 0 then
			return 5
		else
			return 4
		end
	end
end

-- table of dirs in wallmounted order
local wallmounted_to_dir = {
	[0] = vector.new( 0,  1,  0),
	vector.new( 0, -1,  0),
	vector.new( 1,  0,  0),
	vector.new(-1,  0,  0),
	vector.new( 0,  0,  1),
	vector.new( 0,  0, -1),
}
function core.wallmounted_to_dir(wallmounted)
	return wallmounted_to_dir[wallmounted % 8]
end

function core.dir_to_yaw(dir)
	return -math.atan2(dir.x, dir.z)
end

function core.yaw_to_dir(yaw)
	return vector.new(-math.sin(yaw), 0, math.cos(yaw))
end

function core.is_colored_paramtype(ptype)
	return (ptype == "color") or (ptype == "colorfacedir") or
		(ptype == "colorwallmounted") or (ptype == "colordegrotate")
end

function core.strip_param2_color(param2, paramtype2)
	if not core.is_colored_paramtype(paramtype2) then
		return nil
	end
	if paramtype2 == "colorfacedir" then
		param2 = math.floor(param2 / 32) * 32
	elseif paramtype2 == "colorwallmounted" then
		param2 = math.floor(param2 / 8) * 8
	elseif paramtype2 == "colordegrotate" then
		param2 = math.floor(param2 / 32) * 32
	end
	-- paramtype2 == "color" requires no modification.
	return param2
end
