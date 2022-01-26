_G.core = {}

dofile("builtin/common/vector.lua")

core.settings = {
    get = function(self, arg) return 0 end
}

dofile("builtin/game/item.lua")

describe("rotate_facedir()", function()
    it("can reverse rotations", function()
        local in_value = 7
        local out_value = core.rotate_facedir(3, "y+", in_value)
        out_value = core.rotate_facedir(3, "y-", out_value)
        assert.are.equal(in_value, out_value)
    end)

    it("can do quarter rotations", function()
        local in_value = 13
        local out_value = core.rotate_facedir(1, "z+", in_value)
        out_value = core.rotate_facedir(1, "z+", out_value)
        out_value = core.rotate_facedir(1, "z+", out_value)
        out_value = core.rotate_facedir(1, "z+", out_value)
        assert.are.equal(in_value, out_value)
    end)

    it("can do half rotations", function()
        local in_value = 2
        local out_value = core.rotate_facedir(2, "x+", in_value)
        out_value = core.rotate_facedir(2, "x+", out_value)
        assert.are.equal(in_value, out_value)
    end)

    it("can compose rotations well", function()
        local in_value = 6
        local out_value = core.rotate_facedir(1, "x+", in_value)
        out_value = core.rotate_facedir(1, "y+", out_value)
        out_value = core.rotate_facedir(1, "z+", out_value)
        out_value = core.rotate_facedir(1, "y-", out_value)
        out_value = core.rotate_facedir(2, "x-", out_value)
        assert.are.equal(in_value, out_value)
    end)

    it("can construct facedirs", function()
        for in_value = 0, 23 do
            local axis = math.floor(in_value / 4)
            local rotation = in_value % 4
            local out_value = core.rotate_facedir(rotation, "y-")
            if axis == 1 then out_value = core.rotate_facedir(1, "x-", out_value)
            elseif axis == 2 then out_value = core.rotate_facedir(1, "x+", out_value)
            elseif axis == 3 then out_value = core.rotate_facedir(1, "z+", out_value)
            elseif axis == 4 then out_value = core.rotate_facedir(1, "z-", out_value)
            elseif axis == 5 then out_value = core.rotate_facedir(2, "z+", out_value)
            end
            assert.are.equal(in_value, out_value)
        end
    end)
end)
