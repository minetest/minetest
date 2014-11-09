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
	local MaxEdge, MinEdge = self.MaxEdge, self.MinEdge
	return {
		x = MaxEdge.x - MinEdge.x + 1,
		y = MaxEdge.y - MinEdge.y + 1,
		z = MaxEdge.z - MinEdge.z + 1,
	}
end

function VoxelArea:getVolume()
	local e = self:getExtent()
	return e.x * e.y * e.z
end

function VoxelArea:index(x, y, z)
	local MinEdge = self.MinEdge
	local i = (z - MinEdge.z) * self.zstride +
			  (y - MinEdge.y) * self.ystride +
			  (x - MinEdge.x) + 1
	return math.floor(i)
end

function VoxelArea:indexp(p)
	local MinEdge = self.MinEdge
	local i = (p.z - MinEdge.z) * self.zstride +
			  (p.y - MinEdge.y) * self.ystride +
			  (p.x - MinEdge.x) + 1
	return math.floor(i)
end

function VoxelArea:position(i)
	local p = {}
	local MinEdge = self.MinEdge
 
	i = i - 1

	p.z = math.floor(i / self.zstride) + MinEdge.z
	i = i % self.zstride

	p.y = math.floor(i / self.ystride) + MinEdge.y
	i = i % self.ystride

	p.x = math.floor(i) + MinEdge.x

	return p
end

function VoxelArea:contains(x, y, z)
	local MaxEdge, MinEdge = self.MaxEdge, self.MinEdge
	return (x >= MinEdge.x) and (x <= MaxEdge.x) and
		   (y >= MinEdge.y) and (y <= MaxEdge.y) and
		   (z >= MinEdge.z) and (z <= MaxEdge.z)
end

function VoxelArea:containsp(p)
	local MaxEdge, MinEdge = self.MaxEdge, self.MinEdge
	return (p.x >= MinEdge.x) and (p.x <= MaxEdge.x) and
		   (p.y >= MinEdge.y) and (p.y <= MaxEdge.y) and
		   (p.z >= MinEdge.z) and (p.z <= MaxEdge.z)
end

function VoxelArea:containsi(i)
	return (i >= 1) and (i <= self:getVolume())
end

function VoxelArea:iter(minx, miny, minz, maxx, maxy, maxz)
	local i = self:index(minx, miny, minz) - 1
	local last = self:index(maxx, maxy, maxz)
	local ystride = self.ystride
	local zstride = self.zstride
	local yoff = (last+1) % ystride
	local zoff = (last+1) % zstride
	local ystridediff = (i - last) % ystride
	local zstridediff = (i - last) % zstride
	return function()
		i = i + 1
		if i % zstride == zoff then
			i = i + zstridediff
		elseif i % ystride == yoff then
			i = i + ystridediff
		end
		if i <= last then
			return i
		end
	end
end

function VoxelArea:iterp(minp, maxp)
	return self:iter(minp.x, minp.y, minp.z, maxp.x, maxp.y, maxp.z)
end
