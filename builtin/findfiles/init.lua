local postinit = minetest.require('__builtin','postinit')
local inspect = minetest.require('__builtin','inspect')
function printFiles()
   out = io.open(minetest.get_worldpath().."/nodefilemapping.csv","w")
   for n,node in pairs(minetest.registered_nodes) do
      local ti = node.tile_images or node.tiles
      if ti then
         ti = ti[1]
         if ti.name then
            ti = ti.name
         end
         out:write(n..","..ti.."\n")
      end
   end
   out:close()
end

postinit.push(printFiles)
