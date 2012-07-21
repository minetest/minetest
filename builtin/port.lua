local inspect = minetest.require("__builtin","inspect")
local postinit = minetest.require("__builtin","postinit")

local targets = {}
local lookup = {}

local function add(from,to,name,toname)
   if toname == nil then
      toname = name
   end
   local target = from..":"..name
   table.insert(targets,target)
   lookup[target] = to..":"..toname
   minetest.register_node(target,
                          {
                             description = "Porting "..name.." to "..to,
                             tiles = {"unknown.png"},
                             is_ground_content = true,
                             groups = {snappy=2,choppy=3},
                             sounds = default.node_sound_stone_defaults(),
                          })
end

local function start()
   -- for n,v in pairs(lookup) do
   --    print("Porting "..n.." to "..v)
   -- end

   minetest.register_abm(
      { nodenames = targets,
        interval = 10.0,
        chance = 1.0,
        action = function(pos, node, active_object_count, active_object_count_wider)
                    print("GOTCHA "..node.name)
                    minetest.env:add_node(pos,{type="node",name=lookup[node.name]})
                 end
     })
end

postinit.push(start)

return add