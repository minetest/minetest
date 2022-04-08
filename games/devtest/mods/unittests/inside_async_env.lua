unittests = {}

core.log("info", "Hello World")

assert(core == minetest)
assert(not core.get_player_by_name)
assert(not core.set_node)

unittests.async_ok = true -- checked on the other side
