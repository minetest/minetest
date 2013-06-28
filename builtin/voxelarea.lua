VoxelArea = {
	MinEdge = {x=1, y=1, z=1},
	MaxEdge = {x=0, y=0, z=0},
	ystride = 0,
	zstride = 0,
}

function VoxelArea:new(o)
	o = o or {}
	setmetatable(o, self)
	self.__index = self

	local e = o:getExtent()
	o.ystride = e.x
	o.zstride = e.x * e.y

	return o
end

function VoxelArea:getExtent()
	return {
		x = self.MaxEdge.x - self.MinEdge.x + 1,
		y = self.MaxEdge.y - self.MinEdge.y + 1,
		z = self.MaxEdge.z - self.MinEdge.z + 1,
	}
end

function VoxelArea:getVolume()
	local e = self:getExtent()
	return e.x * e.y * e.z
end

function VoxelArea:index(x, y, z)
	local i = (z - self.MinEdge.z) * self.zstride +
			  (y - self.MinEdge.y) * self.ystride +
			  (x - self.MinEdge.x) + 1
	return math.floor(i)
end

function VoxelArea:indexp(p)
	local i = (p.z - self.MinEdge.z) * self.zstride +
			  (p.y - self.MinEdge.y) * self.ystride +
			  (p.x - self.MinEdge.x) + 1
	return math.floor(i)
end

