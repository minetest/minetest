local inspect = minetest.require("__builtin","inspect")
local holdouts = {}
local named = {}
local before = {}

local purging = false

local function push(stage,name,beforer)
   if purging == true then
      error("stupid programmer wants the last spot in line.")
   end
   table.insert(holdouts,stage)
   if name == nil then return end
   named[stage] = name
   if beforer ~= nil then
      if before[name] == nil then
         before[name] = {beforer=true}
      else
         before[name][beforer] = true
      end
   end
end

local fuck = {}

function fallback(a,b)
   if not fuck[a] then
      fuck[a] = #fuck
   end
   if not fuck[b] then
      fuck[b] = #fuck
   end
   return fuck[a] < fuck[b]
end

function isBefore(a,b)
   local name = named[a]
   if name then
      local bff = before[name]
      if not bff then 
         return fallback(a,b) 
      end
      local bname = named[b]
      if not bname then return fallback(a,b) end
      return (not bff[bname])
   else
      return fallback(a,b)
   end
end

local function purge()
   if purging then return end
   purging = true
   table.sort(holdouts,isBefore)
   while #holdouts > 0 do
      local stage
      local temp = {}
      for i,s in ipairs(holdouts) do
         if i == 1 then
            stage = s
         else
            table.insert(temp, s)
         end
      end
      holdouts = temp
      stage()
   end
end

return {
   push=push,
   purge=purge
}