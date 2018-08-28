local stack_mt
stack_mt = {
	__index = {
		push = function(self, v)
			self.n = self.n+1
			self[self.n] = v
		end,
		pop = function(self)
			local v = self[self.n]
			self[self.n] = nil
			self.n = self.n-1
			return v
		end,
		is_empty = function(self)
			return self.n == 0
		end,
		size = function(self)
			return self.n
		end,
		clone = function(self, copy_element)
			local stack
			if copy_element then
				stack = {n = self.n, true}
				for i = 1, self.n do
					stack[i] = copy_element(self[i])
				end
			else
				stack, n = self:to_table()
				stack.n = n
			end
			setmetatable(stack, stack_mt)
			return stack
		end,
		to_table = function(self)
			local t = {}
			for i = 1, self.n do
				t[i] = self[i]
			end
			return t, self.n
		end,
		to_string = function(self, value_tostring)
			if self.n == 0 then
				return "empty stack"
			end
			value_tostring = value_tostring or tostring
			local t = {}
			for i = 1, self.n do
				t[i] = value_tostring(self[i])
			end
			return self.n .. " elements; bottom to top: " ..
				table.concat(t, ", ")
		end,
	}
}

function core.create_stack(data)
	local stack
	if type(data) == "table"
	and data.input then
		stack = data.input
		stack.n = data.n or #data.input
	else
		-- setting the first element to true makes it ~10 times faster with
		-- luajit when the stack always contains less or equal to one element
		stack = {n = 0, true}
	end
	setmetatable(stack, stack_mt)
	return stack
end


local fifo_mt
fifo_mt = {
	__index = {
		add = function(self, v)
			local n = self.n_in+1
			self.n_in = n
			self.sink[n] = v
		end,
		take = function(self)
			local p = self.p_out
			if p <= self.n_out then
				local v = self.source[p]
				self.source[p] = nil
				self.p_out = p+1
				return v
			end
			-- source is empty, swap it with sink
			self.source, self.sink = self.sink, self.source
			self.n_out = self.n_in
			self.n_in = 0
			local v = self.source[1]
			self.source[1] = nil
			self.p_out = 2
			return v
		end,
		is_empty = function(self)
			return self.n_in == 0 and self.p_out == self.n_out+1
		end,
		size = function(self)
			return self.n_in + self.n_out - self.p_out + 1
		end,
		clone = function(self, copy_element)
			local source, n = self:to_table()
			if copy_element then
				for i = 1, n do
					source[i] = copy_element(source[i])
				end
			end
			local fifo = {n_in = 0, n_out = n, p_out = 1,
				sink = {true}, source = source}
			setmetatable(fifo, fifo_mt)
			return fifo
		end,
		to_table = function(self)
			local t = {}
			local k = 1
			for i = self.p_out, self.n_out do
				t[k] = self.source[i]
				k = k+1
			end
			for i = 1, self.n_in do
				t[k] = self.sink[i]
				k = k+1
			end
			return t, k-1
		end,
		to_string = function(self, value_tostring)
			local size = self:size()
			if size == 0 then
				return "empty fifo"
			end
			value_tostring = value_tostring or tostring
			local t = self:to_table()
			for i = 1, #t do
				t[i] = value_tostring(t[i])
			end
			return size .. " elements; oldest to newest: " ..
				table.concat(t, ", ")
		end,
	}
}

function core.create_queue(data)
	local fifo
	if type(data) == "table"
	and data.input then
		fifo = {n_in = 0, n_out = data.n or #data.input, p_out = 1,
			sink = {true}, source = data.input}
	else
		fifo = {n_in = 0, n_out = 0, p_out = 1, sink = {true}, source = {true}}
	end
	setmetatable(fifo, fifo_mt)
	return fifo
end


local function sift_up(binary_heap, i)
	local p = math.floor(i / 2)
	while p > 0
	and binary_heap.compare(binary_heap[i], binary_heap[p]) do
		-- new data has higher priority than its parent
		binary_heap[i], binary_heap[p] = binary_heap[p], binary_heap[i]
		i = p
		p = math.floor(p / 2)
	end
end

local function sift_down(binary_heap, i)
	local n = binary_heap.n
	while true do
		local l = i + i
		local r = l+1
		if l > n then
			break
		end
		if r > n then
			if binary_heap.compare(binary_heap[l], binary_heap[i]) then
				binary_heap[i], binary_heap[l] = binary_heap[l], binary_heap[i]
			end
			break
		end
		local preferred_child =
			binary_heap.compare(binary_heap[l], binary_heap[r]) and l or r
		if not binary_heap.compare(binary_heap[preferred_child],
				binary_heap[i]) then
			break
		end
		binary_heap[i], binary_heap[preferred_child] =
			binary_heap[preferred_child], binary_heap[i]
		i = preferred_child
	end
end

local function build(binary_heap)
	for i = math.floor(binary_heap.n / 2), 1, -1 do
		sift_down(binary_heap, i)
	end
end

local binary_heap_mt
binary_heap_mt = {
	__index = {
		peek = function(self)
			return self[1]
		end,
		add = function(self, v)
			local i = self.n+1
			self.n = i
			self[i] = v
			sift_up(self, i)
		end,
		take = function(self)
			local v = self[1]
			self[1] = self[self.n]
			self[self.n] = nil
			self.n = self.n-1
			sift_down(self, 1)
			return v
		end,
		find = function(self, cond)
			for i = 1, self.n do
				if cond(self[i]) then
					return i
				end
			end
		end,
		change_element = function(self, v, i)
			i = i or 1
			local priority_lower = self.compare(self[i], v)
			self[i] = v
			if priority_lower then
				sift_down(self, i)
			elseif i > 1 then
				sift_up(self, i)
			end
		end,
		merge = function(self, other)
			local n = self.n
			for i = 1, other.n do
				self[n + i] = other[i]
			end
			self.n = n + other.n
			build(self)
		end,
		is_empty = function(self)
			return self.n == 0
		end,
		size = function(self)
			return self.n
		end,
		clone = function(self, copy_element)
			local binary_heap, n = self:to_table()
			if copy_element then
				for i = 1, n do
					binary_heap[i] = copy_element(binary_heap[i])
				end
			end
			binary_heap.n = n
			binary_heap.compare = self.compare
			setmetatable(binary_heap, binary_heap_mt)
			return binary_heap
		end,
		to_table = function(self)
			local t = {}
			for i = 1, self.n do
				t[i] = self[i]
			end
			return t, self.n
		end,
		sort = function(self)
			for i = self.n, 1, -1 do
				self[i], self[1] = self[1], self[i]
				self.n = self.n-1
				sift_down(self, 1)
			end
			setmetatable(self, nil)
			self.compare = nil
		end,
		to_string = function(self, value_tostring)
			if self.n == 0 then
				return "empty binary heap"
			end
			value_tostring = value_tostring or tostring
			local t = {}
			for i = 1, self.n do
				local sep = ""
				if i > 1 then
					sep = (math.log(i) / math.log(2)) % 1 == 0 and "; " or ", "
				end
				t[i] = sep .. value_tostring(self[i])
			end
			return self.n .. " elements: " .. table.concat(t, "")
		end,
	}
}

function core.create_binary_heap(data)
	local compare = data
	if type(data) == "table" then
		if data.input then
			-- make data.elements a binary heap
			local binary_heap = data.input
			binary_heap.n = data.n or #binary_heap
			binary_heap.compare = data.compare
			setmetatable(binary_heap, binary_heap_mt)
			if not data.input_sorted then
				build(binary_heap)
			end
			return
		end
		compare = data.compare
	end
	local binary_heap = {compare = compare, n = 0, true}
	setmetatable(binary_heap, binary_heap_mt)
	return binary_heap
end
