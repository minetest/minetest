_G.core = {}
dofile("builtin/common/vector.lua")
dofile("builtin/common/misc_helpers.lua")

describe("string", function()
	it("trim()", function()
		assert.equal("foo bar", string.trim("\n \t\tfoo bar\t "))
	end)

	describe("split()", function()
		it("removes empty", function()
			assert.same({ "hello" }, string.split("hello"))
			assert.same({ "hello", "world" }, string.split("hello,world"))
			assert.same({ "hello", "world" }, string.split("hello,world,,,"))
			assert.same({ "hello", "world" }, string.split(",,,hello,world"))
			assert.same({ "hello", "world", "2" }, string.split("hello,,,world,2"))
			assert.same({ "hello ", " world" }, string.split("hello :| world", ":|"))
		end)

		it("keeps empty", function()
			assert.same({ "hello" }, string.split("hello", ",", true))
			assert.same({ "hello", "world" }, string.split("hello,world", ",", true))
			assert.same({ "hello", "world", "" }, string.split("hello,world,", ",", true))
			assert.same({ "hello", "", "", "world", "2" }, string.split("hello,,,world,2", ",", true))
			assert.same({ "", "", "hello", "world", "2" }, string.split(",,hello,world,2", ",", true))
			assert.same({ "hello ", " world | :" }, string.split("hello :| world | :", ":|"))
		end)

		it("max_splits", function()
			assert.same({ "one" }, string.split("one", ",", true, 2))
			assert.same({ "one,two,three,four" }, string.split("one,two,three,four", ",", true, 0))
			assert.same({ "one", "two", "three,four" }, string.split("one,two,three,four", ",", true, 2))
			assert.same({ "one", "", "two,three,four" }, string.split("one,,two,three,four", ",", true, 2))
			assert.same({ "one", "two", "three,four" }, string.split("one,,,,,,two,three,four", ",", false, 2))
		end)

		it("pattern", function()
			assert.same({ "one", "two" }, string.split("one,two", ",", false, -1, true))
			assert.same({ "one", "two", "three" }, string.split("one2two3three", "%d", false, -1, true))
		end)
	end)
end)

describe("privs", function()
	it("from string", function()
		assert.same({ a = true, b = true }, core.string_to_privs("a,b"))
	end)

	it("to string", function()
		assert.equal("one", core.privs_to_string({ one=true }))

		local ret = core.privs_to_string({ a=true, b=true })
		assert(ret == "a,b" or ret == "b,a")
	end)
end)

describe("pos", function()
	it("from string", function()
		assert.equal(vector.new(10, 5.1, -2), core.string_to_pos("10.0, 5.1, -2"))
		assert.equal(vector.new(10, 5.1, -2), core.string_to_pos("( 10.0, 5.1, -2)"))
		assert.is_nil(core.string_to_pos("asd, 5, -2)"))
	end)

	it("to string", function()
		assert.equal("(10.1,5.2,-2.3)", core.pos_to_string({ x = 10.1, y = 5.2, z = -2.3}))
	end)
end)

describe("area parsing", function()
	describe("valid inputs", function()
		it("accepts absolute numbers", function()
			local p1, p2 = core.string_to_area("(10.0, 5, -2) (  30.2 4 -12.53)")
			assert(p1.x == 10 and p1.y == 5 and p1.z == -2)
			assert(p2.x == 30.2 and p2.y == 4 and p2.z == -12.53)
		end)

		it("accepts relative numbers", function()
			local p1, p2 = core.string_to_area("(1,2,3) (~5,~-5,~)", {x=10,y=10,z=10})
			assert(type(p1) == "table" and type(p2) == "table")
			assert(p1.x == 1 and p1.y == 2 and p1.z == 3)
			assert(p2.x == 15 and p2.y == 5 and p2.z == 10)

			p1, p2 = core.string_to_area("(1 2 3) (~5 ~-5 ~)", {x=10,y=10,z=10})
			assert(type(p1) == "table" and type(p2) == "table")
			assert(p1.x == 1 and p1.y == 2 and p1.z == 3)
			assert(p2.x == 15 and p2.y == 5 and p2.z == 10)
		end)
	end)
	describe("invalid inputs", function()
		it("rejects too few numbers", function()
			local p1, p2 = core.string_to_area("(1,1) (1,1,1,1)", {x=1,y=1,z=1})
			assert(p1 == nil and p2 == nil)
		end)

		it("rejects too many numbers", function()
			local p1, p2 = core.string_to_area("(1,1,1,1) (1,1,1,1)", {x=1,y=1,z=1})
			assert(p1 == nil and p2 == nil)
		end)

		it("rejects nan & inf", function()
			local p1, p2 = core.string_to_area("(1,1,1) (1,1,nan)", {x=1,y=1,z=1})
			assert(p1 == nil and p2 == nil)

			p1, p2 = core.string_to_area("(1,1,1) (1,1,~nan)", {x=1,y=1,z=1})
			assert(p1 == nil and p2 == nil)

			p1, p2 = core.string_to_area("(1,1,1) (1,~nan,1)", {x=1,y=1,z=1})
			assert(p1 == nil and p2 == nil)

			p1, p2 = core.string_to_area("(1,1,1) (1,1,inf)", {x=1,y=1,z=1})
			assert(p1 == nil and p2 == nil)

			p1, p2 = core.string_to_area("(1,1,1) (1,1,~inf)", {x=1,y=1,z=1})
			assert(p1 == nil and p2 == nil)

			p1, p2 = core.string_to_area("(1,1,1) (1,~inf,1)", {x=1,y=1,z=1})
			assert(p1 == nil and p2 == nil)

			p1, p2 = core.string_to_area("(nan,nan,nan) (nan,nan,nan)", {x=1,y=1,z=1})
			assert(p1 == nil and p2 == nil)

			p1, p2 = core.string_to_area("(nan,nan,nan) (nan,nan,nan)")
			assert(p1 == nil and p2 == nil)

			p1, p2 = core.string_to_area("(inf,inf,inf) (-inf,-inf,-inf)", {x=1,y=1,z=1})
			assert(p1 == nil and p2 == nil)

			p1, p2 = core.string_to_area("(inf,inf,inf) (-inf,-inf,-inf)")
			assert(p1 == nil and p2 == nil)
		end)

		it("rejects words", function()
			local p1, p2 = core.string_to_area("bananas", {x=1,y=1,z=1})
			assert(p1 == nil and p2 == nil)

			p1, p2 = core.string_to_area("bananas", "foobar")
			assert(p1 == nil and p2 == nil)

			p1, p2 = core.string_to_area("bananas")
			assert(p1 == nil and p2 == nil)

			p1, p2 = core.string_to_area("(bananas,bananas,bananas)")
			assert(p1 == nil and p2 == nil)

			p1, p2 = core.string_to_area("(bananas,bananas,bananas) (bananas,bananas,bananas)")
			assert(p1 == nil and p2 == nil)
		end)

		it("requires parenthesis & valid numbers", function()
			local p1, p2 = core.string_to_area("(10.0, 5, -2  30.2,   4, -12.53")
			assert(p1 == nil and p2 == nil)

			p1, p2 = core.string_to_area("(10.0, 5,) -2  fgdf2,   4, -12.53")
			assert(p1 == nil and p2 == nil)
		end)
	end)
end)

describe("table", function()
	it("indexof()", function()
		assert.equal(1, table.indexof({"foo", "bar"}, "foo"))
		assert.equal(-1, table.indexof({"foo", "bar"}, "baz"))
	end)
end)

describe("formspec_escape", function()
	it("escapes", function()
		assert.equal(nil, core.formspec_escape(nil))
		assert.equal("", core.formspec_escape(""))
		assert.equal("\\[Hello\\\\\\[", core.formspec_escape("[Hello\\["))
	end)
end)
