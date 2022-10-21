# Lua code style guidelines

This is largely based on [the Python style guide](https://www.python.org/dev/peps/pep-0008) except for some indentation and language syntax differences.  When in doubt, consult that guide.

Note that these are only *guidelines* for more readable code.  In some (rare) cases they may result in *less* readable code.  Use your best judgement.


## Comments

Incorrect or outdated comments are worse than no comments.

Avoid inline comments, unless they're very short.

Write comments to clarify anything that may be confusing. Don't write comments that describe things that are obvious from the code:

<span style="color: #282;">Good:</span>
```lua
width = width - 2  -- Adjust for 1px border on each side
```

<span style="color: #b44;">Bad:
```lua
width = width - 2  -- Decrement width by two
```

Comments should follow English grammar rules, this means **starting with a capital letter**, using commas and apostrophes where appropriate, and **ending with a period**. The period may be omitted in single-sentence comments. If the first word of a comment is an identifier, its casing should not be changed. **Check the spelling.** Examples:
```lua
-- This is a properly formatted comment.
-- This isnt.              (missing apostrophe)
-- neither is this.        (lowercase first letter)
```

Comments should have a space between the comment characters and the first word:
```lua
--This is wrong.
-- This is right.
```

Inline comments should be separated from the code by two spaces:
```lua
foo()  -- A proper comment.
```

If you write comments for a documentation generation tool, write the comments in LuaDoc format.

Short multi-line comments should use the regular single-line comment style.

Long multi-line comments should use Lua's multi-line comment format with no leading characters except a `--` before the closer:
```lua
--[[
Very long
multi-line comment.
--]]
```


## Lines, spaces, and indentation

Indentation is done with one tab per indentation level.

**Lines are wrapped at 80 characters** where possible, with a hard limit of 90.  If you need more you're probably doing something wrong.


### Continuation lines

Conditional expressions have continuation lines indented by two tabs:
```lua
if long_function_call(with, many, arguments) and
		another_function_call() then
	do_something()
end
```

Function arguments are indented by two tabs if multiple arguments are in a line, same for definition continuations:
```lua
foo(bar, biz, "This is a long string..."
		baz, qux, "Lua")

function foo(a, b, c, d,
		e, f, g, h)
	[…]
end
```

If the function arguments contain a table, it's indented by one tab and if the arguments get own lines, it's indented like a table:
```lua
register_node("name", {
	"This is a long string...",
	0.3,
})

list = filterlist.create(
	preparemodlist,
	comparemod,
	function()
		return "no comma at the end"
	end
)
```

When strings don't fit into the line, you should add the string (changes) to the next line(s) indented by one tab:
```lua
longvarname = longvarname ..
	"Thanks for reading this example!"

local retval =
	"size[11.5,7.5,true]" ..
	"label[0.5,0;" .. fgettext("World:") .. "]" ..
	"label[1.75,0;" .. data.worldspec.name .. "]"
```

When breaking around a binary operator you should break after the operator:
```lua
if a or b or c or d or
		e or f then
	foo()
end
```


### Empty lines

Use a single empty line to separate sections of long functions.

Use two empty lines to separate top-level functions and large tables.

Do not use a empty line after conditional, looping, or function opening statements:

<span style="color: #282;">Good:</span>
```lua
function foo()
	if x then
		bar()
	end
end
```

<span style="color: #b44;">Bad:</span>
```lua
function foo()

	if x then

		bar()
	end
end
```

Don't leave white-space at the end of lines.

### Spaces

Spaces are not used around parenthesis, brackets, or curly braces:

<span style="color: #282;">Good:</span>
```lua
foo({baz=true})
bar[1] = "Hello world"
```

<span style="color: #b44;">Bad:</span>
```lua
foo ( { baz=true } )
bar [ 1 ]
```

Spaces are used after, but not before, commas and semicolons:

<span style="color: #282;">Good:</span>
```lua
foo(a, b, {c, d})
```

<span style="color: #b44;">Bad:</span>
```lua
foo(a,b,{c , d})
```

Spaces are used around binary operators with following exceptions:

* There mustn't be spaces around the member access operator (".")
* Spaces around the concatenation operator ("..") are optional.
* In short one-line table definitions the spaces around the equals sign can be omitted.
* When in-/decrementing a variable by 1, the spaces around the + and - operator can be omitted if you want to "get the neighbour" of something, e.g. when increasing some counter variable.

<span style="color: #282;">Good:</span>
```lua
local num = 2 * (3 / 4)
foo({bar=true})
foo({bar = true})
local def = {
	foo = true,
	bar = false,
}
i = i+1
sometable[#sometable+1] = v
```

<span style="color: #b44;">Bad:</span>
```lua
local num=2*(3/4)
local def={
	foo=true,
	bar=false,
}
playerpos.y = playerpos.y+1  -- playerpos.y is not an integer
```

Use spaces to align related things, but don't go overboard:

<span style="color: #282;">Good:</span>
```lua
local node_up   = minetest.get_node(pos_up)
local node_down = minetest.get_node(pos_down)
-- Too long relative to the other lines, don't align with it
local node_up_1_east_2_north_3 = minetest.get_node(pos_up_1_east_2_north_3)
```

<span style="color: #b44;">Bad:</span>
```lua
local x                       = true
local very_long_variable_name = false

local foobar    = true
local unrelated = {}
```

### Tables

**Small tables may be placed on one line.**  Large tables have one entry per line, with the opening and closing braces on lines without items; and with a comma after the last item:

<span style="color: #282;">Good:</span>
```lua
local foo = {bar=true}
foo = {
	bar = 0,
	biz = 1,
	baz = 2,
}
```

<span style="color: #b44;">Bad:</span>
```lua
foo = {bar = 0,
	biz = 1,
	baz = 2}
foo = {
	bar = 0, biz = 1,
	baz = 2
}
```

In list-style tables where each element is short multiple elements may be placed on each line:
```lua
local first_eight_letters = {
	"a", "b", "c", "d",
	"e", "f", "g", "h",
}
```


## Naming

**Functions and variables should be named in `lowercase_underscore_style`**, with the exception of constructor-like functions such as `PseudoRandom()`, which should use UpperCamelCase.

Don't invent compound words.  Common words like `filename` are okay, but mashes like `getbox` and `collisiondetection` aren't.

Avoid leading and/or trailing underscores.  They're ugly and can be hard to see.


## Misc

Don't put multiple statements on the same line.

You can put conditionals / loops with small conditions and bodies on one line.  This is discouraged for all but the smallest ones though:

<span style="color: #282;">Good:</span>
```lua
local f, err = io.open(filename, "r")
if not f then return err end

if     foo then return foo
elseif bar then return bar
elseif qux then return qux
end
```

<span style="color: #b44;">Bad:</span>
```lua
if not f and use_error then error(err) elseif not f then return err end
```

Don't compare values explicitly to `true`, `false`, or `nil`, unless it's really needed:

<span style="color: #282;">Good:</span>
```lua
local f, err = io.open(filename, "r")
if not f then return err end

local t = {"a", true, false}
for i = 1, 5 do
	-- Needs an explicit nil check to avoid triggering
	-- on false, which is a valid value.
	if t[i] == nil then
		t[i] = "Default"
	end
end
```

<span style="color: #b44;">Bad:</span>
```lua
if f == nil then return err end
```

Don't use unnecessary parenthesis unless they improve readability a lot:
```lua
if y then bar() end -- Good
if (not x) then foo() end -- Bad
```

Write function definitions of the form `function foo()` instead of the lambda form `foo = function()`, except when inserting functions in tables inline, where only the second form will work.

**Avoid globals like the plague.** The only globals that you should create are namespace tables&mdash;and even those might eventually be phased out.

Don't let functions get too large. Maximum length depends on complexity; simple functions can be longer than complex functions.
