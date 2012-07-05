local postinit = minetest.require('__builtin','postinit')
local inspect = minetest.require('__builtin','inspect')
function printFiles()
   out = io.open(minetest.get_worldpath().."/nodefilemapping.csv","w")
   for n,node in pairs(minetest.registered_nodes) do
      if node.walkable then
         print(n)
         local ti = node.tile_images or node.tiles
         out:write(n..","..ti[1].."\n")
      end
   end
   out:close()
end

postinit.push(printFiles)
