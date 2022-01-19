-- The jobs are stored in an array representing a binary min-heap. This heap is
-- a binary tree where no child node has an expiration time before its parent.
-- Parent-child relationships are determined by indices. A parent at index i has
-- children at indices i*2 and i*2+1. Child indices beyond the end of the array
-- represent nonexistent children.
-- All this means that, if the first array element exists, there is no other
-- element in the array which expires sooner.
local jobs = {}

-- Adds a job to the heap.
-- The worst-case complexity is O(log n), where n is the heap size.
local function add_job(job)
	local expire = job.expire
	local index = #jobs + 1
	while index > 1 do
		local parent_index = math.floor(index / 2)
		local parent = jobs[parent_index]
		if expire < parent.expire then
			jobs[index] = parent
			index = parent_index
		else
			break
		end
	end
	jobs[index] = job
end

-- Removes the first (soonest to expire) job from the heap.
-- The worst-case complexity is O(log n), where n is the heap size.
local function remove_first_job()
	local length = #jobs
	local job = jobs[length]
	jobs[length] = nil
	length = length - 1
	if length == 0 then
		return
	end
	local index = 1
	while true do
		local old_index = index
		local expire = job.expire
		local left_index = index * 2
		local right_index = index * 2 + 1
		if left_index <= length then
			local left_expire = jobs[left_index].expire
			if left_expire < expire then
				index = left_index
				expire = left_expire
			end
		end
		if right_index <= length then
			if jobs[right_index].expire < expire then
				index = right_index
			end
		end
		if old_index ~= index then
			jobs[old_index] = jobs[index]
		else
			break
		end
	end
	jobs[index] = job
end

local time = 0.0
local time_next = math.huge

core.register_globalstep(function(dtime)
	time = time + dtime

	if time < time_next then
		return
	end

	-- List of jobs that have to expired; to be built.
	local expired = {}

	-- Remove and collect expired jobs.
	local first_job = jobs[1]
	repeat
		remove_first_job()
		expired[#expired + 1] = first_job
		first_job = jobs[1]
	until not first_job or first_job.expire > time
	time_next = first_job and first_job.expire or math.huge

	-- Run the callbacks afterward to prevent infinite loops with core.after(0, ...).
	for i = 1, #expired do
		local job = expired[i]
		core.set_last_run_mod(job.mod_origin)
		job.func(unpack(job, 1, job.n_args))
	end
end)

local job_metatable = {__index = {}}

local function dummy_func() end
function job_metatable.__index:cancel()
	self.func = dummy_func
	for i = 1, self.n_args do
		self[i] = nil
	end
	self.n_args = 0
end

function core.after(after, func, ...)
	assert(tonumber(after) and not core.is_nan(after) and type(func) == "function",
		"Invalid minetest.after invocation")
	local expire = time + after
	local new_job = {
		expire = expire,
		mod_origin = core.get_last_run_mod(),
		func = func,
		n_args = select("#", ...),
		...
	}
	add_job(new_job)
	time_next = math.min(time_next, expire)
	return setmetatable(new_job, job_metatable)
end
