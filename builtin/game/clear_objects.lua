
-- Minetest: builtin/game/chat.lua

local S = core.get_translator("__builtin")


local raw_clear_objects = core.clear_objects

local function check_rule(rule, value)
	print("Checking rule "..dump(rule).." and value "..dump(value))
	if (type(rule)==type(value)) then
		if (rule==value) then
			return true
		end
	elseif (type(rule)=="table") then
		if (rule.type==type(value)) then
			if (rule.type=="number") then
				if (rule.min~=nil) and (value>=rule.min) then
					if (rule.max~=nil) and (value<=rule.max) then
						return true
					else
						return true
					end
				elseif (rule.max~=nil) and (value<=rule.max) then
					return true
				end
			elseif rule.type=="string" then
				if (rule.match~=nil) and (string.match(value, rule.match)~=nil) then
					return true
				end
				if (rule.find~=nil) and (string.find(value, rule.find)~=nil) then
					return true
				end
			end
		end
	elseif (type(rule)=="string") then
		if (type(value)==rule) then
			return true
		end
	end
	return false
end

core.clear_objects = function(options)
	if (options.mode=="full") or (options.mode=="quick") then
		raw_clear_objects(options)
	elseif (options.mode=="soft") then
		for _, entity in pairs(minetest.luaentities) do
			if not entity.object.get_luaentity().prevent_soft_clearobjects then
				entity.object:remove()
			end
		end
	elseif (options.mode=="rules") then
		local rules = options.rules
		minetest.log("action", "Clearing objects with rules "..dump(rules))
		for _, entity in pairs(minetest.luaentities) do
			if (type(rules.e)=="table") then
				local values = entity.object:get_luaentity()
				for key, rule in pairs(rules.e) do
					if check_rule(rule, values[key]) then
						entity.object:remove()
						break
					end
				end
			end
			if (type(rules.p)=="table") then
				local values = entity.object:get_properties()
				for key, rule in pairs(rules.p) do
					if check_rule(rule, values[key]) then
						entity.object:remove()
						break
					end
				end
			end
		end
	end
end

