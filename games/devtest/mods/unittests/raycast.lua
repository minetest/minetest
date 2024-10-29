local function raycast_with_pointabilities(start_pos, end_pos, pointabilities)
	local ray = core.raycast(start_pos, end_pos, nil, nil, pointabilities)
	for hit in ray do
        if hit.type == "node" then
            return hit.under
        end
	end
    return nil
end

local function test_raycast_pointabilities(player, pos1)
    local pos2 = pos1:offset(0, 0, 1)
    local pos3 = pos1:offset(0, 0, 2)

    local oldnode1 = core.get_node(pos1)
    local oldnode2 = core.get_node(pos2)
    local oldnode3 = core.get_node(pos3)
    core.swap_node(pos1, {name = "air"})
    core.swap_node(pos2, {name = "testnodes:not_pointable"})
    core.swap_node(pos3, {name = "testnodes:pointable"})

    local p = nil
    assert(raycast_with_pointabilities(pos1, pos3, p) == pos3)

    p = core.registered_items["testtools:blocked_pointing_staff"].pointabilities
    assert(raycast_with_pointabilities(pos1, pos3, p) == nil)

    p = core.registered_items["testtools:ultimate_pointing_staff"].pointabilities
    assert(raycast_with_pointabilities(pos1, pos3, p) == pos2)

    core.swap_node(pos1, oldnode1)
    core.swap_node(pos2, oldnode2)
    core.swap_node(pos3, oldnode3)
end

unittests.register("test_raycast_pointabilities", test_raycast_pointabilities, {map=true})
