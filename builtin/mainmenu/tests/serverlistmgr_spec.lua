_G.core = {}
_G.unpack = table.unpack
_G.serverlistmgr = {}

dofile("builtin/common/vector.lua")
dofile("builtin/common/misc_helpers.lua")
dofile("builtin/mainmenu/serverlistmgr.lua")

local base = "builtin/mainmenu/tests/"

describe("legacy favorites", function()
	it("loads well-formed correctly", function()
		local favs = serverlistmgr.read_legacy_favorites(base .. "favorites_wellformed.txt")

		local expected = {
			{
				address = "127.0.0.1",
				port = 30000,
			},

			{ address = "localhost", port = 30000 },

			{ address = "vps.rubenwardy.com", port = 30001 },

			{ address = "gundul.ddnss.de", port = 39155 },

			{
				address = "daconcepts.com",
				port = 30000,
				name = "VanessaE's Dreambuilder creative Server",
				description = "VanessaE's Dreambuilder creative-mode server. Lots of mods, whitelisted buckets."
			},
		}

		assert.same(expected, favs)
	end)
end)
