--PISTONS
--registration normal one:
minetest.register_node("mesecons_pistons:piston_normal", {
	tile_images = {"jeija_piston_tb.png", "jeija_piston_tb.png", "jeija_piston_tb.png", "jeija_piston_tb.png", "jeija_piston_tb.png", "jeija_piston_side.png"},
	groups = {cracky=3},
	paramtype2="facedir",
    	description="Piston",
})

minetest.register_craft({
	output = '"mesecons_pistons:piston_normal" 2',
	recipe = {
		{"default:wood", "default:wood", "default:wood"},
		{"default:cobble", "default:steel_ingot", "default:cobble"},
		{"default:cobble", "mesecons:mesecon_off", "default:cobble"},
	}
})

--registration sticky one:
minetest.register_node("mesecons_pistons:piston_sticky", {
	tile_images = {"jeija_piston_tb.png", "jeija_piston_tb.png", "jeija_piston_tb.png", "jeija_piston_tb.png", "jeija_piston_tb.png", "jeija_piston_sticky_side.png"},
	groups = {cracky=3},
	paramtype2="facedir",
    	description="Sticky Piston",
})

minetest.register_craft({
	output = '"mesecons_pistons:piston_sticky" 1',
	recipe = {
		{'"mesecons_materials:glue"'},
		{'"mesecons_pistons:piston_normal"'},
	}
})

-- get push direction normal
function mesecon:piston_get_direction(pos)
	local direction = {x=0, y=0, z=0}
	if OLD_PISTON_DIRECTION==1 then
		getactivated=0
		local lpos={x=pos.x, y=pos.y, z=pos.z}
		local getactivated=0
		local rules=mesecon:get_rules("piston")
	
		getactivated=getactivated+mesecon:is_power_on(pos, rules[1].x, rules[1].y, rules[1].z)
		if getactivated>0 then direction.y=-1 return direction end
		getactivated=getactivated+mesecon:is_power_on(pos, rules[2].x, rules[2].y, rules[2].z)
		if getactivated>0 then direction.y=1 return direction end
		for k=3, 5 do
			getactivated=getactivated+mesecon:is_power_on(pos, rules[k].x, rules[k].y, rules[k].z)
		end
		if getactivated>0 then direction.z=1 return direction end
	
		for n=6, 8 do
			getactivated=getactivated+mesecon:is_power_on(pos, rules[n].x, rules[n].y, rules[n].z)
		end

		if getactivated>0 then direction.z=-1 return direction end
	
		for j=9, 11 do
			getactivated=getactivated+mesecon:is_power_on(pos, rules[j].x, rules[j].y, rules[j].z)
		end

		if getactivated>0 then direction.x=-1 return direction end

		for l=12, 14 do
			getactivated=getactivated+mesecon:is_power_on(pos, rules[l].x, rules[l].y, rules[l].z)
		end
		if getactivated>0 then direction.x=1 return direction end
	else
		local node=minetest.env:get_node(pos)
		if node.param2==3 then
			return {x=1, y=0, z=0}
		end
		if node.param2==2 then
			return {x=0, y=0, z=1}
		end
		if node.param2==1 then
			return {x=-1, y=0, z=0}
		end
		if node.param2==0 then
			return {x=0, y=0, z=-1}
		end
	end
	return direction
end

-- get pull/push direction sticky
function mesecon:sticky_piston_get_direction(pos)
	if OLD_PISTON_DIRECTION==1 then
		getactivated=0
		local direction = {x=0, y=0, z=0}
		local lpos={x=pos.x, y=pos.y, z=pos.z}
		local getactivated=0
		local rules=mesecon:get_rules("piston")

		getactivated=getactivated+mesecon:is_power_off(pos, rules[1].x, rules[1].y, rules[1].z)
		if getactivated>0 then direction.y=-1 return direction end
		getactivated=getactivated+mesecon:is_power_off(pos, rules[2].x, rules[2].y, rules[2].z)
		if getactivated>0 then direction.y=1 return direction end

		for k=3, 5 do
			getactivated=getactivated+mesecon:is_power_off(pos, rules[k].x, rules[k].y, rules[k].z)
		end
		if getactivated>0 then direction.z=1 return direction end

		for n=6, 8 do
			getactivated=getactivated+mesecon:is_power_off(pos, rules[n].x, rules[n].y, rules[n].z)
		end

		if getactivated>0 then direction.z=-1 return direction end
	
		for j=9, 11 do
			getactivated=getactivated+mesecon:is_power_off(pos, rules[j].x, rules[j].y, rules[j].z)
		end

		if getactivated>0 then direction.x=-1 return direction end
	
		for l=12, 14 do
			getactivated=getactivated+mesecon:is_power_off(pos, rules[l].x, rules[l].y, rules[l].z)
		end
		if getactivated>0 then direction.x=1 return direction end
	else
		local node=minetest.env:get_node(pos)
		if node.param2==3 then
			return {x=1, y=0, z=0}
		end
		if node.param2==2 then
			return {x=0, y=0, z=1}
		end
		if node.param2==1 then
			return {x=-1, y=0, z=0}
		end
		if node.param2==0 then
			return {x=0, y=0, z=-1}
		end
	end
	return direction
end

-- Push action
mesecon:register_on_signal_on(function (pos, node)
	if (node.name=="mesecons_pistons:piston_normal" or node.name=="mesecons_pistons:piston_sticky") then
		local direction=mesecon:piston_get_direction(pos)

		local checknode={}
		local checkpos={x=pos.x, y=pos.y, z=pos.z}
		repeat -- Check if it collides with a stopper
			checkpos={x=checkpos.x+direction.x, y=checkpos.y+direction.y, z=checkpos.z+direction.z}
			checknode=minetest.env:get_node(checkpos)
			if mesecon:is_mvps_stopper(checknode.name) then 
				return 
			end
		until checknode.name=="air"
		or checknode.name=="ignore" 
		or checknode.name=="default:water" 
		or checknode.name=="default:water_flowing" 

		local obj={}
		if node.name=="mesecons_pistons:piston_normal" then
			obj=minetest.env:add_entity(pos, "mesecons_pistons:piston_pusher_normal")
		elseif node.name=="mesecons_pistons:piston_sticky" then
			obj=minetest.env:add_entity(pos, "mesecons_pistons:piston_pusher_sticky")
		end
		
		if ENABLE_PISTON_ANIMATION==1 then		
			obj:setvelocity({x=direction.x*4, y=direction.y*4, z=direction.z*4})
		else
			obj:moveto({x=pos.x+direction.x, y=pos.y+direction.y, z=pos.z+direction.z}, false)
		end
		
		local np = {x=pos.x+direction.x, y=pos.y+direction.y, z=pos.z+direction.z}	
		local coln = minetest.env:get_node(np)
		
		or checknode.name=="ignore" 
		or checknode.name=="default:water" 
		or checknode.name=="default:water_flowing" 

		if coln.name ~= "air" and coln.name ~="water" then
			local thisp= {x=np.x, y=np.y, z=np.z}
			local thisnode=minetest.env:get_node(thisp)
			local nextnode={}
			minetest.env:remove_node(thisp)
			repeat
				thisp.x=thisp.x+direction.x
				thisp.y=thisp.y+direction.y
				thisp.z=thisp.z+direction.z
				nextnode=minetest.env:get_node(thisp)
				minetest.env:add_node(thisp, {name=thisnode.name})
				nodeupdate(thisp)
				thisnode=nextnode
			until thisnode.name=="air" 
			or thisnode.name=="ignore" 
			or thisnode.name=="default:water" 
			or thisnode.name=="default:water_flowing" 
		end
	end
end)

--Pull action (sticky only)
mesecon:register_on_signal_off(function (pos, node)
	if node.name=="mesecons_pistons:piston_sticky" or node.name=="mesecons_pistons:piston_normal" then
		local objs = minetest.env:get_objects_inside_radius(pos, 2)
		for k, obj in pairs(objs) do
			local obj_name = obj:get_entity_name()
			if obj_name == "mesecons_pistons:piston_pusher_normal" or obj_name == "mesecons_pistons:piston_pusher_sticky" then
				obj:remove()
			end
		end

		if node.name=="mesecons_pistons:piston_sticky" then
			local direction=mesecon:sticky_piston_get_direction(pos)
			local np = {x=pos.x+direction.x, y=pos.y+direction.y, z=pos.z+direction.z}	
			local coln = minetest.env:get_node(np)
			if coln.name == "air" or coln.name =="water" then
				local thisp= {x=np.x+direction.x, y=np.y+direction.y, z=np.z+direction.z}
				local thisnode=minetest.env:get_node(thisp)
				if thisnode.name~="air" and thisnode.name~="water" and not mesecon:is_mvps_stopper(thisnode.name) then
					local newpos={}
					local oldpos={}
					minetest.env:add_node(np, {name=thisnode.name})
					minetest.env:remove_node(thisp)
				end		
			end
		end
	end
end)

--Piston Animation
local PISTON_PUSHER_NORMAL={
	physical = false,
	visual = "sprite",
	textures = {"default_wood.png", "default_wood.png", "jeija_piston_pusher_normal.png", "jeija_piston_pusher_normal.png", "jeija_piston_pusher_normal.png", "jeija_piston_pusher_normal.png"},
	collisionbox = {-0.5,-0.5,-0.5, 0.5,0.5,0.5},
	visual = "cube",
	timer=0,
}

function PISTON_PUSHER_NORMAL:on_step(dtime)
	self.timer=self.timer+dtime
	if self.timer>=0.24 then
		self.object:setvelocity({x=0, y=0, z=0})
	end
end

local PISTON_PUSHER_STICKY={
	physical = false,
	visual = "sprite",
	textures = {"default_wood.png", "default_wood.png", "jeija_piston_pusher_sticky.png", "jeija_piston_pusher_sticky.png", "jeija_piston_pusher_sticky.png", "jeija_piston_pusher_sticky.png"},
	collisionbox = {-0.5,-0.5,-0.5, 0.5,0.5,0.5},
	visual = "cube",
	timer=0,
}

function PISTON_PUSHER_STICKY:on_step(dtime)
	self.timer=self.timer+dtime
	if self.timer>=0.24 then
		self.object:setvelocity({x=0, y=0, z=0})
	end
end

minetest.register_entity("mesecons_pistons:piston_pusher_normal", PISTON_PUSHER_NORMAL)
minetest.register_entity("mesecons_pistons:piston_pusher_sticky", PISTON_PUSHER_STICKY)

minetest.register_on_dignode(function(pos, node)
	if node.name=="mesecons_pistons:piston_normal" or node.name=="mesecons_pistons:piston_sticky" then
		local objs = minetest.env:get_objects_inside_radius(pos, 2)
		for k, obj in pairs(objs) do
			local obj_name = obj:get_entity_name()
			if obj_name == "mesecons_pistons:piston_pusher_normal" or obj_name == "mesecons_pistons:piston_pusher_sticky" then
				obj:remove()
			end
		end
	end
end)
