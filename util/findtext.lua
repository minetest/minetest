#! /usr/bin/env lua

local me = arg[0]:gsub(".*[/\\](.*)$", "%1")

local function err(fmt, ...)
	io.stderr:write(("%s: %s\n"):format(me, fmt:format(...)))
	os.exit(1)
end

local output
local inputs = { }
local lang
local author

local i = 1

local function usage()
	print([[
Usage: ]]..me..[[ [OPTIONS] FILE...

Extract translatable strings from the given FILE(s).

Available options:
  -h,--help         Show this help screen and exit.
  -o,--output X     Set output file (default: stdout).
  -a,--author X     Set author.
  -l,--lang X       Set language name.
]])
	os.exit(0)
end

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
	elseif (a == "-a") or (a == "--author") then
		i = i + 1
		if i > #arg then
			err("missing required argument to `%s'", a)
		end
		author = arg[i]
	elseif (a == "-l") or (a == "--lang") then
		i = i + 1
		if i > #arg then
			err("missing required argument to `%s'", a)
		end
		lang = arg[i]
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

if author or lang then
	outfile:write("\n")
end

if lang then
	printf("# Language: %s\n", lang)
end

if author then
	printf("# Author: %s\n", author)
end

if author or lang then
	outfile:write("\n")
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

local messages = {}

for _, file in ipairs(inputs) do
	local infile, e = io.open(file, "r")
	local textdomains = {}
	if infile then
		for line in infile:lines() do
			for translator_name, textdomain in line:gmatch('local (%w+)%s*=%s*%w+%.get_translator%("([^"]*)"%)') do
				--print(translator_name, textdomain)
				messages[textdomain] = messages[textdomain] or {}
				textdomains[translator_name] = textdomain
			end
			for translator, s in line:gmatch('(%w+)%("([^"]*)"') do
				if textdomains[translator] then
					local textdomain = textdomains[translator]
					table.insert(messages[textdomain], s)
				end
			end
			for textdomain, s in line:gmatch('%w+%.translate%("([^"]*)"%s*,%s*"([^"]*)"') do
				messages[textdomain] = messages[textdomain] or {}
				table.insert(messages[textdomain], s)
			end
		end
		infile:close()
	else
		io.stderr:write(("%s: WARNING: error opening file: %s\n"):format(me, e))
	end
end

for textdomain, mtbl in pairs(messages) do
	table.sort(messages[textdomain])

	local last_msg
	printf("# textdomain: %s\n", textdomain)

	for _, msg in ipairs(messages[textdomain]) do
		if msg ~= last_msg then
			printf("%s=\n", escape(msg))
		end
		last_msg = msg
	end
end

if output then
	outfile:close()
end

--[[
TESTS:
local S = minetest.get_translator("domain")
S("foo") S("bar")
S("bar")
S("foo")
minetest.translate("another_domain", "foo")
S("#foo=@1\n@2", "bar", "baz")
]]
