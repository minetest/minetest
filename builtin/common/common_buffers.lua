-- Reusable common buffers for use with e.g. VoxelManip:get_data(<buffer>)

local common_buffers = {}
function core.get_common_buffer(index)
	if index == nil then
		index = 1
	elseif type(index) ~= "number" then
		error("Invalid buffer index; expected number, got " .. type(index))
	end

	local buffer = common_buffers[index]
	if not buffer then
		buffer = {}
		common_buffers[index] = buffer
	end
	return buffer
end
