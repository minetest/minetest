minetest.register_node("mesecons_noteblock:noteblock", {
	description = "Noteblock",
	tile_images = {"mesecons_noteblock.png"},
	groups = {snappy=2,choppy=2,oddly_breakable_by_hand=2},
	drawtype = "allfaces_optional",
	visual_scale = 1.3,
	paramtype="light",
})

minetest.register_on_placenode(function (pos, node)
	if node.name=="mesecons_noteblock:noteblock" then
		minetest.env:add_node(pos, {name=node.name, param2=0})
	end
end)

minetest.register_on_punchnode(function (pos, node)
	if node.name=="mesecons_noteblock:noteblock" then
		local param2 = node.param2+1
		if param2==12 then param2=0 end
		minetest.env:add_node(pos, {name=node.name, param2=param2})
		mesecon.noteblock_play(pos, param2)
	end
end)

mesecon.noteblock_play = function (pos, param2)
	local soundname
	if param2==8 then
		soundname="mesecons_noteblock_a"
	elseif param2==9 then
		soundname="mesecons_noteblock_asharp"
	elseif param2==10 then
		soundname="mesecons_noteblock_b"
	elseif param2==11 then
		soundname="mesecons_noteblock_c"
	elseif param2==0 then
		soundname="mesecons_noteblock_csharp"
	elseif param2==1 then
		soundname="mesecons_noteblock_d"
	elseif param2==2 then
		soundname="mesecons_noteblock_dsharp"
	elseif param2==3 then
		soundname="mesecons_noteblock_e"
	elseif param2==4 then
		soundname="mesecons_noteblock_f"
	elseif param2==5 then
		soundname="mesecons_noteblock_fsharp"
	elseif param2==6 then
		soundname="mesecons_noteblock_g"
	elseif param2==7 then
		soundname="mesecons_noteblock_gsharp"
	end
	local block_below_name = minetest.env:get_node({x=pos.x, y=pos.y-1, z=pos.z}).name
	if block_below_name == "default:glass" then
		soundname="mesecons_noteblock_hihat"
	end
	if block_below_name == "default:stone" then
		soundname="mesecons_noteblock_kick"
	end
	if block_below_name == "default:chest" then
		soundname="mesecons_noteblock_snare"
	end
	if block_below_name == "default:tree" then
		soundname="mesecons_noteblock_crash"
	end
	if block_below_name == "default:wood" then
		soundname="mesecons_noteblock_litecrash"
	end
	minetest.sound_play(soundname,
	{pos = pos, gain = 1.0, max_hear_distance = 32,})
end

mesecon:register_on_signal_on(function(pos, node)
	if node.name=="mesecons_noteblock:noteblock" then
		mesecon.noteblock_play(pos, node.param2)
	end
end)
