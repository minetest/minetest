local armor_stand_formspec = "size[8,7]" ..
	default.gui_bg ..
	default.gui_bg_img ..
	default.gui_slots ..
	default.get_hotbar_bg(0,3) ..
	"list[current_name;armor_head;3,0.5;1,1;]" ..
	"list[current_name;armor_torso;4,0.5;1,1;]" ..
	"list[current_name;armor_legs;3,1.5;1,1;]" ..
	"list[current_name;armor_feet;4,1.5;1,1;]" ..
	"image[3,0.5;1,1;3d_armor_stand_head.png]" ..
	"image[4,0.5;1,1;3d_armor_stand_torso.png]" ..
	"image[3,1.5;1,1;3d_armor_stand_legs.png]" ..
	"image[4,1.5;1,1;3d_armor_stand_feet.png]" ..
	"list[current_player;main;0,3;8,1;]" ..
	"list[current_player;main;0,4.25;8,3;8]"

local elements = {"head", "torso", "legs", "feet"}

local function update_entity(pos)
	local object = nil
	local node = minetest.get_node(pos)
	local objects = minetest.get_objects_inside_radius(pos, 1) or {}
	pos = vector.round(pos)
	for _, obj in pairs(objects) do
		local p = vector.round(obj:getpos())
		if vector.equals(p, pos) then
			local ent = obj:get_luaentity()
			if ent then
				if ent.name == "3d_armor_stand:armor_entity" then
					-- Remove duplicates
					if object then
						obj:remove()
					else
						object = obj
					end
				end
			end
		end
	end
	if object then
		if node.name ~= "3d_armor_stand:armor_stand" and node.name ~= "3d_armor_stand:locked_armor_stand" then
			object:remove()
			return
		end
	else
		object = minetest.add_entity(pos, "3d_armor_stand:armor_entity")
	end
	if object then
		local texture = "3d_armor_trans.png"
		local textures = {}
		local meta = minetest.get_meta(pos)
		local inv = meta:get_inventory()
		local yaw = 0
		if inv then
			for _, element in pairs(elements) do
				local stack = inv:get_stack("armor_"..element, 1)
				if stack:get_count() == 1 then
					local item = stack:get_name() or ""
					local def = stack:get_definition() or {}
					local texture = def.texture or item:gsub("%:", "_")
					table.insert(textures, texture..".png")
				end
			end
		end
		if #textures > 0 then
			texture = table.concat(textures, "^")
		end
		if node.param2 then
			local rot = node.param2 % 4
			if rot == 1 then
				yaw = 3 * math.pi / 2
			elseif rot == 2 then
				yaw = math.pi
			elseif rot == 3 then
				yaw = math.pi / 2
			end
		end
		object:setyaw(yaw)
		object:set_properties({textures={texture}})
	end
end

minetest.register_node("3d_armor_stand:armor_stand", {
	description = "Armor stand",
	drawtype = "mesh",
	mesh = "3d_armor_stand.obj",
	tiles = {"default_wood.png", "default_steel_block.png"},
	paramtype = "light",
	paramtype2 = "facedir",
	walkable = false,
	selection_box = {
		type = "fixed",
		fixed = {-0.5,-0.5,-0.5, 0.5,1.4,0.5}
	},
	groups = {choppy=2, oddly_breakable_by_hand=2},
	sounds = default.node_sound_wood_defaults(),
	on_construct = function(pos)
		local meta = minetest.get_meta(pos)
		meta:set_string("formspec", armor_stand_formspec)
		meta:set_string("infotext", "Armor Stand")
		local inv = meta:get_inventory()
		for _, element in pairs(elements) do
			inv:set_size("armor_"..element, 1)
		end
	end,
	can_dig = function(pos, player)
		local meta = minetest.get_meta(pos)
		local inv = meta:get_inventory()
		for _, element in pairs(elements) do
			if not inv:is_empty("armor_"..element) then
				return false
			end
		end
		return true
	end,
	after_place_node = function(pos)
		minetest.add_entity(pos, "3d_armor_stand:armor_entity")
	end,
	allow_metadata_inventory_put = function(pos, listname, index, stack)
		local def = stack:get_definition() or {}
		local groups = def.groups or {}
		if groups[listname] then
			return 1
		end
		return 0
	end,
	allow_metadata_inventory_move = function(pos)
		return 0
	end,
	on_metadata_inventory_put = function(pos)
		update_entity(pos)
	end,
	on_metadata_inventory_take = function(pos)
		update_entity(pos)
	end,
	after_destruct = function(pos)
		update_entity(pos)
	end,
})

local function has_locked_armor_stand_privilege(meta, player)
	local name = ""
	if player then
		if minetest.check_player_privs(player, "protection_bypass") then
			return true
		end
		name = player:get_player_name()
	end
	if name ~= meta:get_string("owner") then
		return false
	end
	return true
end

local node = {}
for k,v in pairs(minetest.registered_nodes["3d_armor_stand:armor_stand"]) do node[k] = v end
node.description = "Locked " .. node.description
node.allow_metadata_inventory_put = function(pos, listname, index, stack, player)
	local meta = minetest.get_meta(pos)
	if not has_locked_armor_stand_privilege(meta, player) then
		return 0
	end
	local def = stack:get_definition() or {}
	local groups = def.groups or {}
	if groups[listname] then
		return 1
	end
	return 0
end
node.allow_metadata_inventory_take = function(pos, listname, index, stack, player)
	local meta = minetest.get_meta(pos)
	if not has_locked_armor_stand_privilege(meta, player) then
		return 0
	end
	return stack:get_count()
end
node.on_construct = function(pos)
		local meta = minetest.get_meta(pos)
		meta:set_string("formspec", armor_stand_formspec)
		meta:set_string("infotext", "Armor Stand")
		meta:set_string("owner", "")
		local inv = meta:get_inventory()
		for _, element in pairs(elements) do
			inv:set_size("armor_"..element, 1)
		end
	end
node.after_place_node = function(pos, placer)
	minetest.add_entity(pos, "3d_armor_stand:armor_entity")
	local meta = minetest.get_meta(pos)
	meta:set_string("owner", placer:get_player_name() or "")
	meta:set_string("infotext", "Armor Stand (owned by " ..
	meta:get_string("owner") .. ")")
end
minetest.register_node("3d_armor_stand:locked_armor_stand", node)

minetest.register_entity("3d_armor_stand:armor_entity", {
	physical = true,
	visual = "mesh",
	mesh = "3d_armor_entity.obj",
	visual_size = {x=1, y=1},
	collisionbox = {-0.1,-0.4,-0.1, 0.1,1.3,0.1},
	textures = {"3d_armor_trans.png"},
	on_activate = function(self)
		local pos = self.object:getpos()
		update_entity(pos)
	end,
})

minetest.register_craft({
	output = "3d_armor_stand:armor_stand",
	recipe = {
		{"", "default:fence_wood", ""},
		{"", "default:fence_wood", ""},
		{"default:steel_ingot", "default:steel_ingot", "default:steel_ingot"},
	}
})

minetest.register_craft({
	output = "3d_armor_stand:locked_armor_stand",
	recipe = {
		{"3d_armor_stand:armor_stand", "default:steel_ingot"},
	}
})
