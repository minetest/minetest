local S, NS = minetest.get_translator("testtranslations")

local function send_compare(name, text)
	minetest.chat_send_player(name, ("%s | %s"):format(text, minetest.get_translated_string("fr", text)))
end

minetest.register_chatcommand("testtranslations", {
	params = "",
	description = "Test translations",
	privs = {},
	func = function(name, param)
		minetest.chat_send_player(name, "Please ensure your locale is set to \"fr\"")
		minetest.chat_send_player(name, "Client-side translation | Server-side translation (fr)")
		send_compare(name, S("Testing .tr files: untranslated"))
		send_compare(name, S("Testing .po files: untranslated"))
		send_compare(name, S("Testing .mo files: untranslated"))
		for i = 0,4 do
			send_compare(name, NS("@1: .po singular", "@1: .po plural", i, tostring(i)))
			send_compare(name, NS("@1: .mo singular", "@1: .mo plural", i, tostring(i)))
		end
	end
})
