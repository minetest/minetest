local S, NS = minetest.get_translator("testtranslations")

minetest.register_chatcommand("testtranslations", {
	params = "",
	description = "Test translations",
	privs = {},
	func = function(name, param)
		minetest.chat_send_player(name, "Please ensure your locale is set to \"fr\"")
		minetest.chat_send_player(name, S("Testing .tr files: untranslated"))
		minetest.chat_send_player(name, S("Testing .po files: untranslated"))
		minetest.chat_send_player(name, S("Testing .mo files: untranslated"))
		for i = 0,4 do
			minetest.chat_send_player(name, NS("@1: .po singular", "@1: .po plural", i, tostring(i)))
			minetest.chat_send_player(name, NS("@1: .mo singular", "@1: .mo plural", i, tostring(i)))
		end
	end
})
