#! /usr/bin/env lua

local me = arg[0]:gsub(".*[/\\](.*)$", "%1")

local function err(fmt, ...)
	io.stderr:write(("%s: %s\n"):format(me, fmt:format(...)))
	os.exit(1)
end

local output, outfile, template
local catalogs = { }

local function usage()
	print([[
Usage: ]]..me..[[ [OPTIONS] TEMPLATE CATALOG...

Update a catalog with new strings from a template.

Available options:
  -h,--help         Show this help screen and exit.
  -o,--output X     Set output file (default: stdout).

Messages in the template that are not on the catalog are added to the
catalog at the end.

This tool also checks messages that are in the catalog but not in the
template, and reports such lines. It's up to the user to remove such
lines, if so desired.
]])
	os.exit(0)
end

local i = 1

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
	elseif a:sub(1, 1) ~= "-" then
		if not template then
			template = a
		else
			table.insert(catalogs, a)
		end
	else
		err("unrecognized option `%s'", a)
	end
	i = i + 1
end

if not template then
	err("no template specified")
elseif #catalogs == 0 then
	err("no catalogs specified")
end

local f, e = io.open(template, "r")
if not f then
	err("error opening template: %s", e)
end

local escapes = {
	["\n"] = "@n",
	["="] = "@=",
}

local function escape(s)
	local r = s:gsub("\\n", "@n"):gsub("[\n=]", escapes)
	if r == "" or not r:sub(1, 1) == "#" then
		return r
	else
		-- Escape '#' at beginning of line
		return "@" .. r
	end
end

if output then
	outfile, e = io.open(output, "w")
	if not outfile then
		err("error opening file for writing: %s", e)
	end
end

local function load_strings(file)
	local infile, e = io.open(file, "r")
	local messages = {}
	local textdomain = ""
	messages[""] = {}
	if infile then
		for line in infile:lines() do
			for td in line:gmatch('# textdomain: (%S+)') do
				textdomain = td
				messages[textdomain] = messages[textdomain] or {}
			end
			if not (line == "" or line:sub(1, 1) == "#") then
				local i = 1
				while i < line:len() do
					if line:sub(i, i) == "@" then
						i = i + 2
					elseif line:sub(i, i) == "=" then
						break
					else
						i = i + 1
					end
				end
				local untranslated = line:sub(1, i - 1)
				local translated = line:sub(i + 1)
				messages[textdomain][untranslated] = translated
				print(file, textdomain, untranslated, translated)
			end
		end
		infile:close()
	else
		io.stderr:write(("%s: WARNING: error opening file: %s\n"):format(me, e))
	end
	return messages
end


local template_msgs = load_strings(template)
for _, file in ipairs(catalogs) do
	print("Processing: "..file)
	local catalog_msgs = load_strings(file)
	local dirty_lines = {}
	local dirty = false
	if catalog_msgs then
		-- Add new entries from template.
		for textdomain, tm in pairs(template_msgs) do
			for k in pairs(tm) do
				if not catalog_msgs[textdomain][k] then
					print("NEW: "..textdomain.." "..k)
					dirty_lines[textdomain] = dirty_lines[textdomain] or {}
					table.insert(dirty_lines[textdomain], k.."=")
					dirty = true
				end
			end
		end
		-- Check for old messages.
		for textdomain, cm in pairs(catalog_msgs) do
			for k, v in pairs(cm) do
				if not template_msgs[textdomain][k] then
					print("OLD: "..textdomain.." "..k)
					dirty_lines[textdomain] = dirty_lines[textdomain] or {}
					table.insert(dirty_lines[textdomain], "# OLD: "..k.."="..v)
					dirty = true
				end
			end
		end
		if dirty then
			local outf
			outf, e = io.open(file, "a+")
			if outf then
				for textdomain, dl in pairs(dirty_lines) do
					outf:write("\n")
					outf:write("# textdomain: " .. textdomain .. "\n")
					for _, line in ipairs(dl) do
						outf:write(line)
						outf:write("\n")
					end
				end
				outf:close()
			else
				io.stderr:write(("%s: WARNING: cannot write: %s\n"):format(me, e))
			end
		end
	else
		io.stderr:write(("%s: WARNING: could not load catalog\n"):format(me))
	end
end
