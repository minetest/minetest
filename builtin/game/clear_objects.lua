
-- Minetest: builtin/game/chat.lua

local S = core.get_translator("__builtin")

--[[
-- rules table format:
{
	-- optional table with rules for check object properties table values
	p = {
		-- direct comparison, value type in property table match with type in rules table
		max_hp = 5,
		-- also direct comparison with value from property table
		info = "Info test",
		-- check if value in property table is number between min and max values range.
		-- It is allowed to specify only min or max limit.
		max_breath = {
			type = "number",
			min = 1,
			max = 5,
		},
		-- Check if value in property table is string which match value.
		-- Field match in used in string.match call as second parameter.
		nametag = {
			type = "string",
			match = "Match%d",
		},
		-- Check if value in property table is string which contains string.
		nametag = {
			type = "string",
			find = "Find",
		},
	},
	-- optional table with rules for check luaentity table values
	-- rules specification is same as for p
	e = {
		-- direct comparasion, type in entity table match with type in rules table
		name = "modname:entityname",
	},
}
--]]

local raw_clear_objects = core.clear_objects

local function check_rule(rule, value)
	--print("Checking rule "..dump(rule).." and value "..dump(value))
	if type(rule)==type(value) then
		-- direct comparison, rule and value have equel type
		if rule==value then
			return true
		end
	elseif type(rule)=="table" then
		-- rule table, check type of value
		if rule.type==type(value) then
			if rule.type=="number" then
				-- rule for number, check value min/max limits
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
				-- rule for string, check if value match or contain specified substring
				if (rule.match~=nil) and (string.match(value, rule.match)~=nil) then
					return true
				end
				if (rule.find~=nil) and (string.find(value, rule.find)~=nil) then
					return true
				end
			end
		end
	end
	return false
end

core.clear_objects = function(options)
	if (options.mode=="full") or (options.mode=="quick") then
		-- C++ powered clear objects function, fast, bud not selective
		raw_clear_objects(options)
	elseif (options.mode=="soft") then
		-- soft mode remove all entities without prevet_soft_clearobjects field
		for _, entity in pairs(minetest.luaentities) do
			if not entity.object:get_luaentity().prevent_soft_clearobjects then
				entity.object:remove()
			end
		end
	elseif (options.mode=="rules") then
		-- rules mod apply rules to decide if object should be removed or kept
		-- object is removed only when all rules match
		local rules = options.rules
		local remove_init = ((type(rules.e)=="table") and (not rawequal(next(rules.e), nil)))
		                    or ((type(rules.p)=="table") and (not rawequal(next(rules.p), nil)))
		if remove_init then
			minetest.log("action", S("Clearing objects with rules: @1", dump(rules)))
			for _, entity in pairs(minetest.luaentities) do
				local remove = remove_init
				-- rules for luaentity table
				if (type(rules.e)=="table") then
					local values = entity.object:get_luaentity()
					for key, rule in pairs(rules.e) do
						remove = remove and check_rule(rule, values[key])
						if not remove then break end
					end
				end
				-- rules for properties table
				if (type(rules.p)=="table") then
					local values = entity.object:get_properties()
					for key, rule in pairs(rules.p) do
						remove = remove and check_rule(rule, values[key])
						if not remove then break end
					end
				end
				if remove then
					entity.object:remove()
				end
			end
		else
			minetest.log("action", S("No usefull rules for clearing objects has been set."))
		end
	end
end

