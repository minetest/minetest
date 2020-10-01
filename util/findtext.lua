#!/usr/bin/env lua

local me = arg[0]:gsub(".*[/\\](.*)$", "%1")

local function err(fmt, ...)
	io.stderr:write(("%s: %s\n"):format(me, fmt:format(...)))
	os.exit(1)
end

local output
local inputs = { }

local i = 1

local function usage()
	print([[
Usage: ]]..me..[[ [OPTIONS] FILE...
Extract translatable strings from the given FILE(s).
Available options:
  -h,--help         Show this help screen and exit.
  -o,--output X     Set output file (default: stdout).
     --tr           Use old .tr format.
     --po           Use .po format (default).
]])
	os.exit(0)
end

local format = "po"

while i <= #arg do
	local a = arg[i]
	if (a == "-h") or (a == "--help") then
		usage()
	elseif (a == "-o") or (a == "--output") then
		i = i + 1
		if i > #arg then
			err("missing required argument to `%s'", a)
		end
		output = arg[i]
        elseif (a == "--tr") then
                format = "tr"
        elseif (a == "--po") then
                format = "po"
	elseif a:sub(1, 1) ~= "-" then
		table.insert(inputs, a)
	else
		err("unrecognized option `%s'", a)
	end
	i = i + 1
end

if #inputs == 0 then
	err("no input files")
end

local outfile = io.stdout

local function printf(fmt, ...)
	outfile:write(fmt:format(...))
end

if output then
	local e
	outfile, e = io.open(output, "w")
	if not outfile then
		err("error opening file for writing: %s", e)
	end
end

local escapes = {
	["\n"] = "@n",
	["="] = "@=",
}

local function escape(s)
	local r = s:gsub("\\n", "@n"):gsub("[\n=]", escapes)
	if r == "" or r:sub(1, 1) ~= "#" then
		return r
	else
		-- Escape '#' at beginning of line
		return "@" .. r
	end
end

local function insert(tbl, x)
	if tbl.n == nil then
		tbl.n = #tbl
	end
	tbl.n = tbl.n + 1
	tbl[tbl.n] = x
end

local messages = {}
local messages_n = {}

for _, file in ipairs(inputs) do
	local infile, e = io.open(file, "r")
	local textdomains = {}
	local textdomains_n = {}
	if infile then
		for line in infile:lines() do
			for translator_name, textdomain in line:gmatch('%f[%w_.]local%s+([%w_]+)%s*=%s*[%w_]+%.get_translator%("([^"]*)"%)') do
				messages[textdomain] = messages[textdomain] or {}
				textdomains[translator_name] = textdomain
			end
			for translator_name, translator_name_n, textdomain in line:gmatch('%f[%w_.]local%s+([%w_]+)%s*,%s*([%w_]+)%s*=%s*[%w_]+%.get_translator%("([^"]*)"%)') do
				messages[textdomain] = messages[textdomain] or {}
				messages_n[textdomain] = messages_n[textdomain] or {}
				textdomains[translator_name] = textdomain
				textdomains_n[translator_name_n] = textdomain
			end
			for translator, s in line:gmatch('%f[%w_.]([%w_]+)%("([^"]*)"') do
				if textdomains[translator] then
					local textdomain = textdomains[translator]
					insert(messages[textdomain], s)
				end
			end
			for translator, s1, s2 in line:gmatch('%f[%w_.]([%w_]+)%("([^"]*)"%s*,%s*"([^"]*)"') do
				if textdomains_n[translator] then
					local textdomain = textdomains_n[translator]
					insert(messages_n[textdomain], { singular = s1, plural = s2 })
				end
			end
			for textdomain, s in line:gmatch('[%w_]+%.translate%("([^"]*)"%s*,%s*"([^"]*)"') do
				messages[textdomain] = messages[textdomain] or {}
				insert(messages[textdomain], s)
			end
			for textdomain, s1, s2 in line:gmatch('[%w_]+%.translate_n%("([^"]*)"%s*,%s*"([^"]*)"%s*,%s*"([^"]*)"') do
				messages_n[textdomain] = messages_n[textdomain] or {}
				insert(messages_n[textdomain], { singular = s1, plural = s2 })
			end
		end
		infile:close()
	else
		io.stderr:write(("%s: WARNING: error opening file: %s\n"):format(me, e))
	end
end

if format == "tr" then

	for textdomain, mtbl in pairs(messages_n) do
		for _, msg in ipairs(mtbl) do
			print("Error: translations with plurals are unsupported in the .tr format")
			os.exit(0)
		end
	end

	for textdomain, mtbl in pairs(messages) do
		table.sort(mtbl)

		local last_msg
		printf("# textdomain: %s\n", textdomain)

		for _, msg in ipairs(mtbl) do
			if msg ~= last_msg then
				printf("%s=\n", escape(msg))
			end
			last_msg = msg
		end
	end

elseif format == "po" then

	for textdomain, mtbl in pairs(messages_n) do
		for _, msg in ipairs(mtbl) do
			printf('msgctx "%s"\n', textdomain)
			printf('msgid "%s"\n', msg.singular)
			printf('msgid_plural "%s"\n', msg.plural)
			printf('\n')
		end
	end

	for textdomain, mtbl in pairs(messages) do
		table.sort(mtbl)

		local last_msg

		for _, msg in ipairs(mtbl) do
			if msg ~= last_msg then
				printf('msgctx "%s"\n', textdomain)
				printf('msgid "%s"\n', msg)
				printf('\n')
			end
			last_msg = msg
		end
	end

end

if output then
	outfile:close()
end

--[[
TESTS:
local S1 = minetest.get_translator("domain1")
local S2, NS2 = minetest.get_translator("domain2")
S1("foo") S2("bar") NS2("bar", "baz", 42)
S2("bar") minetest.translate_n("domain3", "spam", "eggs", 17)
S2("foo")
minetest.translate("domain3", "foo")
S1("#foo=@1\n@2", "bar", "baz")
]]
