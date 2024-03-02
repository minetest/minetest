local S = core.get_translator("unittests")

local function test_server_translation()
	local function translate(str)
		return core.get_translated_string("de:fr", S(str))
	end
	local RESULT_PRIMARY = "Result in primary language."
	local RESULT_SECONDARY = "Result in secondary language."
	assert(translate("Only in primary language.") == RESULT_PRIMARY, "missing translation for primary language")
	assert(translate("Only in secondary language.") == RESULT_SECONDARY, "missing translation for secondary language")
	assert(translate("Available in both languages.") == RESULT_PRIMARY, "incorrect translation priority list applied")
end
unittests.register("test_server_translation", test_server_translation)
