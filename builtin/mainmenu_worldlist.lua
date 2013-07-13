worldlist = {}

--------------------------------------------------------------------------------
function worldlist.refresh()
	worldlist.m_raw_worldlist = engine.get_worlds()
	worldlist.process()
end

--------------------------------------------------------------------------------
function worldlist.init()
	worldlist.m_gamefilter = nil
	worldlist.m_sortmode = "alphabetic"

	worldlist.m_processed_worldlist = nil
	worldlist.m_raw_worldlist = engine.get_worlds()

	worldlist.process()
end

--------------------------------------------------------------------------------
function worldlist.set_gamefilter(gameid)
	if gameid == worldlist.m_gamefilter then
		return
	end
	worldlist.m_gamefilter = gameid
	worldlist.process()
end

--------------------------------------------------------------------------------
function worldlist.get_gamefilter()
	return worldlist.m_gamefilter
end

--------------------------------------------------------------------------------
--supported sort mode "alphabetic|none"
function worldlist.set_sortmode(mode)
	if (mode == worldlist.m_sortmode) then
		return
	end
	worldlist.m_sortmode = mode
	worldlist.process()
end

--------------------------------------------------------------------------------
function worldlist.get_list()
	return worldlist.m_processed_worldlist
end

--------------------------------------------------------------------------------
function worldlist.get_raw_list()
	return worldlist.m_raw_worldlist
end

--------------------------------------------------------------------------------
function worldlist.get_raw_world(idx)
	if type(idx) ~= "number" then
		idx = tonumber(idx)
	end
	
	if idx ~= nil and idx > 0 and idx < #worldlist.m_raw_worldlist then
		return worldlist.m_raw_worldlist[idx]
	end
	
	return nil
end

--------------------------------------------------------------------------------
function worldlist.get_engine_index(worldlistindex)
	assert(worldlist.m_processed_worldlist ~= nil)
	
	if worldlistindex ~= nil and worldlistindex > 0 and
		worldlistindex <= #worldlist.m_processed_worldlist then
		local entry = worldlist.m_processed_worldlist[worldlistindex]
		
		for i,v in ipairs(worldlist.m_raw_worldlist) do
		
			if worldlist.compare(v,entry) then
				return i
			end
		end
	end
	
	return 0
end

--------------------------------------------------------------------------------
function worldlist.get_current_index(worldlistindex)
	assert(worldlist.m_processed_worldlist ~= nil)
	
	if worldlistindex ~= nil and worldlistindex > 0 and
		worldlistindex <= #worldlist.m_raw_worldlist then
		local entry = worldlist.m_raw_worldlist[worldlistindex]
		
		for i,v in ipairs(worldlist.m_processed_worldlist) do
		
			if worldlist.compare(v,entry) then
				return i
			end
		end
	end
	
	return 0
end

--------------------------------------------------------------------------------
function worldlist.process()
	assert(worldlist.m_raw_worldlist ~= nil)

	if worldlist.m_sortmode == "none" and 
	   worldlist.m_gamefilter == nil then
		worldlist.m_processed_worldlist = worldlist.m_raw_worldlist
		return
	end
	
	worldlist.m_processed_worldlist = {}
	
	for i,v in ipairs(worldlist.m_raw_worldlist) do
	
		if worldlist.m_gamefilter == nil or 
			v.gameid == worldlist.m_gamefilter then
			table.insert(worldlist.m_processed_worldlist,v)
		end
	end
	
	if worldlist.m_sortmode == "none" then
		return
	end
	
	if worldlist.m_sortmode == "alphabetic" then
		worldlist.sort_alphabetic()
	end

end

--------------------------------------------------------------------------------
function worldlist.compare(world1,world2)

	if world1.path ~= world2.path then
		return false
	end
	
	if world1.name ~= world2.name then
		return false
	end
	
	if world1.gameid ~= world2.gameid then
		return false
	end

	return true
end

--------------------------------------------------------------------------------
function worldlist.size()
	if worldlist.m_processed_worldlist == nil then
		return 0
	end
	
	return #worldlist.m_processed_worldlist
end

--------------------------------------------------------------------------------
function worldlist.exists(worldname)
	for i,v in ipairs(worldlist.m_raw_worldlist) do
		if v.name == worldname then
			return true
		end
	end
	return false
end


--------------------------------------------------------------------------------
function worldlist.engine_index_by_name(worldname)
	local worldcount = 0
	local worldidx = 0
	for i,v in ipairs(worldlist.m_raw_worldlist) do
		if v.name == worldname then
			worldcount = worldcount +1
			worldidx = i
		end
	end
	
	
	-- If there are more worlds than one with same name we can't decide which
	-- one is meant. This shouldn't be possible but just for sure.
	if worldcount > 1 then
		worldidx=0
	end

	return worldidx
end

--------------------------------------------------------------------------------
function worldlist.sort_alphabetic() 

	table.sort(worldlist.m_processed_worldlist, function(a, b) 
			local n1 = a.name 
			local n2 = b.name 
			local count = math.min(#n1, #n2) 
			
			for i=1,count do 
				if n1:sub(i, i):lower() < n2:sub(i, i):lower() then 
					return true 
				elseif n1:sub(i, i):lower() > n2:sub(i, i):lower() then 
					return false 
				end 
			end 
			return (#n1 <= #n2) 
		end) 
end
