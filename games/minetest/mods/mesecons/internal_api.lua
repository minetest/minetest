-- INTERNAL API

function mesecon:is_receptor_node(nodename, pos, ownpos) --ownpos must be position of the effector/mesecon NOT of the receptor node; pos is the receptor position
	local i=1
	local j=1
	repeat
		if mesecon.pwr_srcs[i].name==nodename then
			if pos==nil and ownpos==nil then --old usage still possible
				return true
			end
			local rules=mesecon.pwr_srcs[i].rules
			local node=minetest.env:get_node(pos)
			if rules==nil then
				rules=mesecon.pwr_srcs[i].get_rules(node.param2)
			end

			j=1
			while rules[j]~=nil do --Check if dest. position is specified in the receptor's rules
				if pos.x+rules[j].x==ownpos.x
				and pos.y+rules[j].y==ownpos.y
				and pos.z+rules[j].z==ownpos.z then
					return true
				end
				j=j+1
			end
		end
		i=i+1
	until mesecon.pwr_srcs[i]==nil
	return false
end

function mesecon:is_receptor_node_off(nodename, pos, ownpos) --ownpos must be position of the effector/mesecon NOT of the receptor node; pos is the receptor position
	local i=1
	local j=1
	repeat
		if mesecon.pwr_srcs_off[i].name==nodename then
			if pos==nil and ownpos==nil then --old usage still possible
				return true
			end
			local rules=mesecon.pwr_srcs_off[i].rules
			local node=minetest.env:get_node(pos)
			if rules==nil then
				rules=mesecon.pwr_srcs_off[i].get_rules(node.param2)
			end

			j=1
			while rules[j]~=nil do
				if pos.x+rules[j].x==ownpos.x
				and pos.y+rules[j].y==ownpos.y
				and pos.z+rules[j].z==ownpos.z then
					return true
				end
				j=j+1
			end
		end
		i=i+1
	until mesecon.pwr_srcs_off[i]==nil
	return false
end

--Signals

function mesecon:activate(pos)
	local node = minetest.env:get_node(pos)	
	local i = 1
	repeat
		i=i+1
		if mesecon.actions_on[i]~=nil then mesecon.actions_on[i](pos, node) 
		else break			
		end
	until false
end

function mesecon:deactivate(pos)
	local node = minetest.env:get_node(pos)	
	local i = 1
	repeat
		i=i+1
		if mesecon.actions_off[i]~=nil then mesecon.actions_off[i](pos, node) 
		else break			
		end
	until false
end

function mesecon:changesignal(pos)
	local node = minetest.env:get_node(pos)	
	local i = 1
	repeat
		i=i+1
		if mesecon.actions_change[i]~=nil then mesecon.actions_change[i](pos, node) 
		else break			
		end
	until false
end

--Rules

function mesecon:add_rules(name, rules)
	local i=0
	while mesecon.rules[i]~=nil do
		i=i+1
	end
	mesecon.rules[i]={}
	mesecon.rules[i].name=name
	mesecon.rules[i].rules=rules
end

function mesecon:get_rules(name)
	local i=0
	while mesecon.rules[i]~=nil do
		if mesecon.rules[i].name==name then
			return mesecon.rules[i].rules
		end
		i=i+1
	end
end

--Conductor system stuff

function mesecon:register_conductor(onstate, offstate)
	local i=0
	while mesecon.conductors[i]~=nil do
		i=i+1
	end
	mesecon.conductors[i]={}
	mesecon.conductors[i].on=onstate
	mesecon.conductors[i].off=offstate
end

function mesecon:get_conductor_on(offstate)
	local i=0
	while mesecon.conductors[i]~=nil do
		if mesecon.conductors[i].off==offstate then
			return mesecon.conductors[i].on
		end
		i=i+1
	end
	return false
end

function mesecon:get_conductor_off(onstate)
	local i=0
	while mesecon.conductors[i]~=nil do
		if mesecon.conductors[i].on==onstate then
			return mesecon.conductors[i].off
		end
		i=i+1
	end
	return false
end

function mesecon:is_conductor_on(name)
	local i=0
	while mesecon.conductors[i]~=nil do
		if mesecon.conductors[i].on==name then
			return true
		end
		i=i+1
	end
	return false
end

function mesecon:is_conductor_off(name)
	local i=0
	while mesecon.conductors[i]~=nil do
		if mesecon.conductors[i].off==name then
			return true
		end
		i=i+1
	end
	return false
end


--Rules rotation Functions:
function mesecon:rotate_rules_right(rules)
	local i=1
	local nr={};
	while rules[i]~=nil do
		nr[i]={}
		nr[i].z=rules[i].x
		nr[i].x=-rules[i].z
		nr[i].y=rules[i].y
		i=i+1
	end
	return nr
end

function mesecon:rotate_rules_left(rules)
	local i=1
	local nr={};
	while rules[i]~=nil do
		nr[i]={}
		nr[i].z=-rules[i].x
		nr[i].x=rules[i].z
		nr[i].y=rules[i].y
		i=i+1
	end
	return nr
end

function mesecon:rotate_rules_down(rules)
	local i=1
	local nr={};
	while rules[i]~=nil do
		nr[i]={}
		nr[i].y=rules[i].x
		nr[i].x=-rules[i].y
		nr[i].z=rules[i].z
		i=i+1
	end
	return nr
end

function mesecon:rotate_rules_up(rules)
	local i=1
	local nr={};
	while rules[i]~=nil do
		nr[i]={}
		nr[i].y=-rules[i].x
		nr[i].x=rules[i].y
		nr[i].z=rules[i].z
		i=i+1
	end
	return nr
end

function mesecon:is_power_on(pos)
	local node = minetest.env:get_node(pos)
	if node.name == "mesecons:mesecon_on" or mesecon:is_receptor_node(node.name) then
		return true
	end
	return false
end

function mesecon:is_power_off(pos)
	local node = minetest.env:get_node(pos)
	if mesecon:is_conductor_off(node.name) or mesecon:is_receptor_node_off(node.name) then
		return 1
	end
	return 0
end

function mesecon:turnon(p, x, y, z, firstcall, rules)
	if rules==nil then
		rules=mesecon:get_rules("default")
	end
	local lpos = {}
	lpos.x=p.x+x
	lpos.y=p.y+y
	lpos.z=p.z+z

	mesecon:changesignal(lpos)
	mesecon:activate(lpos)

	local node = minetest.env:get_node(lpos)
	if mesecon:is_conductor_off(node.name) then
		minetest.env:add_node(lpos, {name=mesecon:get_conductor_on(node.name)})
		nodeupdate(lpos)
	end
	if mesecon:is_conductor_off(node.name) or firstcall then
		local i=1
		while rules[i]~=nil do 
			mesecon:turnon(lpos, rules[i].x, rules[i].y, rules[i].z, false)
			i=i+1
		end
	end
end

function mesecon:turnoff(pos, x, y, z, firstcall, rules)
	if rules==nil then
		rules=mesecon:get_rules("default")
	end
	local lpos = {}
	lpos.x=pos.x+x
	lpos.y=pos.y+y
	lpos.z=pos.z+z

	local node = minetest.env:get_node(lpos)
	local connected = 0
	local checked = {}

	--Send Signals to effectors:
	mesecon:changesignal(lpos)
	if not mesecon:check_if_turnon(lpos) then
		mesecon:deactivate(lpos)
	end

	if not(firstcall) and connected==0 then
		connected=mesecon:connected_to_pw_src(lpos, 0, 0, 0, checked)	
	end

	if connected == 0 and  mesecon:is_conductor_on(node.name) then
		--minetest.env:remove_node(lpos)
		minetest.env:add_node(lpos, {name=mesecon:get_conductor_off(node.name)})
		nodeupdate(lpos)
	end


	if mesecon:is_conductor_on(node.name) or firstcall then
		if connected == 0 then
			local i=1
			while rules[i]~=nil do 
				mesecon:turnoff(lpos, rules[i].x, rules[i].y, rules[i].z, false)
				i=i+1
			end
		end
	end
end


function mesecon:connected_to_pw_src(pos, x, y, z, checked, firstcall)
	local i=1
	local lpos = {}

	lpos.x=pos.x+x
	lpos.y=pos.y+y
	lpos.z=pos.z+z

	
	local node = minetest.env:get_node_or_nil(lpos)

	if not(node==nil) then
		repeat
			i=i+1
			if checked[i]==nil then checked[i]={} break end
			if  checked[i].x==lpos.x and checked[i].y==lpos.y and checked[i].z==lpos.z then 
				return 0
			end
		until false

		checked[i].x=lpos.x
		checked[i].y=lpos.y
		checked[i].z=lpos.z

		if mesecon:is_receptor_node(node.name, lpos, pos) == true then -- receptor nodes (power sources) can be added using mesecon:add_receptor_node
			return 1
		end

		if mesecon:is_conductor_on(node.name) or firstcall then -- add other conductors here
				local pw_source_found=0				
				local rules=mesecon:get_rules("default")
				local i=1
				while rules[i]~=nil do 
					pw_source_found=pw_source_found+mesecon:connected_to_pw_src(lpos, rules[i].x, rules[i].y, rules[i].z, checked, false)
					i=i+1
				end
			if pw_source_found > 0 then
				return 1
			end 
		end
	end
	return 0
end

function mesecon:check_if_turnon(pos)
	local i=1
	local j=1
	local k=1
	local rcpt
	local rcpt_pos={}
	local rules

	rules=mesecon:get_rules("default") --Power form a on-conductor
	while rules[k]~=nil do
		if mesecon:is_conductor_on(minetest.env:get_node({x=pos.x+rules[k].x, y=pos.y+rules[k].y, z=pos.z+rules[k].z}).name) then
			return true
		end
		k=k+1
	end

	while mesecon.rules[i]~=nil do --Power from a receptor
		j=1
		while mesecon.rules[i].rules[j]~=nil do
			rcpt_pos={x=pos.x-mesecon.rules[i].rules[j].x, y=pos.y-mesecon.rules[i].rules[j].y, z=pos.z-mesecon.rules[i].rules[j].z}
			rcpt=minetest.env:get_node(rcpt_pos)
			if mesecon:is_receptor_node(rcpt.name, rcpt_pos, pos) then 
				return true 
			end
			j=j+1
		end
		i=i+1
	end
	return false
end

minetest.register_on_placenode(function(pos, newnode, placer)
	if mesecon:check_if_turnon(pos) then
		if mesecon:is_conductor_off(newnode.name) then
			mesecon:turnon(pos, 0, 0, 0)		
		else
			mesecon:changesignal(pos)
			mesecon:activate(pos)
		end
	end
end)

minetest.register_on_dignode(
	function(pos, oldnode, digger)
		if mesecon:is_conductor_on(oldnode.name) then
			mesecon:turnoff(pos, 0, 0, 0, true)
		end	
	end
)
