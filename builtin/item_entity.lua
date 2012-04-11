function minetest.spawn_item(pos, item)
        -- Take item in any format
        local stack = ItemStack(item)
        local obj = minetest.env:add_entity(pos, "__builtin:item")
        obj:get_luaentity():set_item(stack:to_string())
        return obj
end
 
minetest.register_entity("__builtin:item", {
        initial_properties = {
                hp_max = 1,
                physical = true,
                collisionbox = {-0.17,-0.17,-0.17, 0.17,0.17,0.17},
                visual = "sprite",
                visual_size = {x=0.5, y=0.5},
                textures = {""},
                spritediv = {x=1, y=1},
                initial_sprite_basepos = {x=0, y=0},
                is_visible = false,
        },
       
        itemstring = '',
        physical_state = true,
        dontbugme = true,
        hasplayer = false,
 
        set_item = function(self, itemstring)
                self.itemstring = itemstring
                local stack = ItemStack(itemstring)
                local itemtable = stack:to_table()
                local itemname = nil
                if itemtable then
                        itemname = stack:to_table().name
                end
                local item_texture = nil
                local item_type = ""
                if minetest.registered_items[itemname] then
                        item_texture = minetest.registered_items[itemname].inventory_image
                        item_type = minetest.registered_items[itemname].type
                end
                prop = {
                        is_visible = true,
                        visual = "sprite",
                        textures = {"unknown_item.png"}
                }
                if item_texture and item_texture ~= "" then
                        prop.visual = "sprite"
                        prop.textures = {item_texture}
                        prop.visual_size = {x=0.50, y=0.50}
                else
                        prop.visual = "wielditem"
                        prop.textures = {itemname}
                        prop.visual_size = {x=0.20, y=0.20}
                        prop.automatic_rotate = math.pi * 0.25
                end
                self.object:set_properties(prop)
        end,
 
        get_staticdata = function(self)
                return self.itemstring
        end,
 
        on_activate = function(self, staticdata)
                self.itemstring = staticdata
                self.object:set_armor_groups({immortal=1})
                self.object:setvelocity({x=0, y=5, z=0})
                self.object:setacceleration({x=0, y=-10, z=0})
                self:set_item(self.itemstring)
        end,
 
        on_step = function(self, dtime)
                local p = self.object:getpos()
                p.y = p.y - 0.3
                local nn = minetest.env:get_node(p).name
                if minetest.registered_nodes[nn].walkable then
                        if self.physical_state then
                                self.object:setvelocity({x=0,y=0,z=0})
                                self.object:setacceleration({x=0, y=0, z=0})
                                self.physical_state = false
                                self.object:set_properties({
                                        physical = false
                                })
                                self.dontbugme = false
                        end
                else
                        if not self.physical_state then
                                self.object:setvelocity({x=0,y=0,z=0})
                                self.object:setacceleration({x=0, y=-10, z=0})
                                self.physical_state = true
                                self.object:set_properties({
                                        physical = true
                                })
                                self.dontbugme = true
                        end
                end
                local pos = p
                local outercircle = 1.5
                local maxaccell = 1
                local innercircle = .5
                local objs = minetest.env:get_objects_inside_radius(pos, innercircle)
                local objs2 = minetest.env:get_objects_inside_radius(pos, outercircle)
                for k, obj in pairs(objs) do
                        local objpos=obj:getpos()
                        if objpos.y>pos.y-1 and objpos.y<pos.y+0.5 then
                                if obj:get_player_name() ~= nil then
                                        if self.itemstring ~= '' then
                                                obj:get_inventory():add_item("main", self.itemstring)
                                        end
                                        self.object:remove()
                                end
                        end
                end
                if self.dontbugme == false then
                        for k, obj in pairs(objs2) do
                                local objpos=obj:getpos()
                                if obj:get_player_name() ~= nil then
                                        print(obj:get_player_name())
                                        isplayerin2 = true
                                        playerobj2 = obj
                                        local fx = obj:getpos().x - pos.x
                                        local fy = obj:getpos().y - pos.y
                                        local fz = obj:getpos().z - pos.z
                                        local fix = maxaccell - fx
                                        fix = fix * -1
                                        local fiz = maxaccell - fz
                                        fiz = fiz * -1
                                        self.object:setacceleration({x=0, y=0, z=0})
                                        self.object:setvelocity({x=fx * 5, y=fy * 5, z=fz * 5})
                                        return
                                end
                        end
                end
        end,
 
        on_punch = function(self, hitter)
                if self.itemstring ~= '' then
                        hitter:get_inventory():add_item("main", self.itemstring)
                end
                self.object:remove()
        end,
})