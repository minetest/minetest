local function simple_list_iter(t)
	local n = 0
	return function()
		n = n+1
		return t[n]
	end
end

-- useful to short chat autocompletion code
function core.autocomplete_word(start, t, iter)
	if not iter then
		if t[1] then
			iter = simple_list_iter
		else
			iter = pairs
		end
	end

	local start_len = #start
	local possible_words,n = {},0
	for word in iter(t) do
		if word:sub(1, start_len) == start then
			n = n+1
			possible_words[n] = word
		end
	end

	if n < 2 then
		return possible_words[1]
	end

	table.sort(possible_words)
	local first_word = possible_words[1]
	local last_word = possible_words[#possible_words]
	local l = start_len
	for i = l+1, math.min(#first_word, #last_word) do
		if first_word:sub(i, i) ~= last_word:sub(i, i) then
			break
		end
		l = l+1
	end

	if l ~= start_len then
		return first_word:sub(1, l), possible_words
	end

	return nil, possible_words
end

-- Add playernames autocompletion
core.after(0, core.register_chat_autocompletion,
	function(message, _, first_invocation)
		local len = #message
		local last_space = len - (message:reverse():find(" ") or len + 1)

		local start
		if last_space + 1 == len then
			start = ""
		else
			start = message:sub(last_space - len + 1)
		end

		local pnames = core.get_player_names()
		local replacement, possible_players = core.autocomplete_word(start, pnames)

		if not replacement then
			return
		end

		if type(replacement) == "string" then
			message = message:sub(1, last_space - len) .. replacement
			if not possible_players then
				message = message .. " "
			end
			return message, #message, true
		end

		if not first_invocation then
			core.display_chat_message(core.gettext("Players found: ") ..
					table.concat(replacement, ", "))
		end
	end
)
