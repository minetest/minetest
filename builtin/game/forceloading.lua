-- Prevent anyone else accessing those functions
local forceload_block = core.forceload_block
local forceload_free_block = core.forceload_free_block
core.forceload_block = nil
core.forceload_free_block = nil

local blocks_forceloaded
local total_forceloaded = 0

local BLOCKSIZE = 16
local function get_blockpos(pos)
	return {
		x = math.floor(pos.x/BLOCKSIZE),
		y = math.floor(pos.y/BLOCKSIZE),
		z = math.floor(pos.z/BLOCKSIZE)}
end

function core.forceload_block(pos)
	local blockpos = get_blockpos(pos)
	local hash = core.hash_node_position(blockpos)
	if blocks_forceloaded[hash] ~= nil then
		blocks_forceloaded[hash] = blocks_forceloaded[hash] + 1
		return true
	else
		if total_forceloaded >= (tonumber(core.setting_get("max_forceloaded_blocks")) or 16) then
			return false
		end
		total_forceloaded = total_forceloaded+1
		blocks_forceloaded[hash] = 1
		forceload_block(blockpos)
		return true
	end
end

function core.forceload_free_block(pos)
	local blockpos = get_blockpos(pos)
	local hash = core.hash_node_position(blockpos)
	if blocks_forceloaded[hash] == nil then return end
	if blocks_forceloaded[hash] > 1 then
		blocks_forceloaded[hash] = blocks_forceloaded[hash] - 1
	else
		total_forceloaded = total_forceloaded-1
		blocks_forceloaded[hash] = nil
		forceload_free_block(blockpos)
	end
end

-- Keep the forceloaded areas after restart
local wpath = core.get_worldpath()
local function read_file(filename)
	local f = io.open(filename, "r")
	if f==nil then return {} end
	local t = f:read("*all")
	f:close()
	if t=="" or t==nil then return {} end
	return core.deserialize(t) or {}
end

local function write_file(filename, table)
	local f = io.open(filename, "w")
	f:write(core.serialize(table))
	f:close()
end

blocks_forceloaded = read_file(wpath.."/force_loaded.txt")
for _, __ in pairs(blocks_forceloaded) do
	total_forceloaded = total_forceloaded + 1
end

core.after(5, function()
	for hash, _ in pairs(blocks_forceloaded) do
		local blockpos = core.get_position_from_hash(hash)
		forceload_block(blockpos)
	end
end)

core.register_on_shutdown(function()
	write_file(wpath.."/force_loaded.txt", blocks_forceloaded)
end)
