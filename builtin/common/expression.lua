local vector_indices = {
	x = true,
	y = true,
	z = true,
}

expression = {}
local expr_meta = {}
function expr_meta.__index(self, idx)
	if vector_indices[idx] then
		return expression.index(self, idx)
	end
	return expr_meta[idx]
end

local function with_expr_meta(table)
	return setmetatable(table, expr_meta)
end

-- Convert to an expression
function expression.expression(value)
	local value_type = type(value)
	if value_type == "number" then
		-- Constant scalars
		return with_expr_meta({
			expr_type = "constant_scalar",
			value = value,
		})
	elseif value_type == "table" then
		if getmetatable(value) == expr_meta then
			-- This is an expression already
			return value
		elseif value.x and value.y and value.z then
			-- Constant (Minetest) vectors
			return with_expr_meta({
				expr_type = "vector",
				x = expression.expression(value.x),
				y = expression.expression(value.y),
				z = expression.expression(value.z),
			})
		end
	end
	error("Value of invalid type used as expression")
end

local unary_operations = {
	acos = true,
	asin = true,
	atan = true,
	ceil = true,
	cos = true,
	floor = true,
	log = true,
	sin = true,
	tan = true,
}

function expression.unary_operation(op, arg)
	assert(unary_operations[op])

	local expr_arg = expression.expression(arg)

	return with_expr_meta({
		expr_type = "unary_operation",
		operation = op,
		arg = expr_arg,
	})
end

local binary_operations = {
	add = true,
	subtract = true,
	multiply = true,
	divide = true,
	power = true,
	random = true,
	lt = true,
	le = true,
	atan2 = true,
}

function expression.binary_operation(op, arg1, arg2)
	assert(binary_operations[op])

	local expr_arg1 = expression.expression(arg1)
	local expr_arg2 = expression.expression(arg2)

	return with_expr_meta({
		expr_type = "binary_operation",
		operation = op,
		arg1 = expr_arg1,
		arg2 = expr_arg2,
	})
end

function expression.index(arg, index)
	assert(vector_indices[index])

	local vec = expression.expression(arg)

	return with_expr_meta({
		expr_type = "index",
		vector = vec,
		index = index,
	})
end

function expression.time()
	return with_expr_meta({
		expr_type = "time",
	})
end

function expression.random(low, high)
	return expression.binary_operation("random", low or 0, high or 1)
end

local function binop_fun(op)
	return function(arg1, arg2)
		return expression.binary_operation(op, arg1, arg2)
	end
end

expr_meta.__add = binop_fun("add")
expr_meta.__sub = binop_fun("subtract")
expr_meta.__mul = binop_fun("multiply")
expr_meta.__div = binop_fun("divide")
expr_meta.__pow = binop_fun("power")
expression.lt = binop_fun("lt")
expression.le = binop_fun("le")

function expression.gt(a, b)
	return expression.lt(b, a)
end

function expression.ge(a, b)
	return expression.le(b, a)
end

function expr_meta.__unm(arg1)
	return 0 - arg1
end

function expr_meta.__newindex()
	error("Cannot set values in expressions.")
end

function expr_meta:add_ref_counts(count_table)
	local old_value = count_table[self] or 0
	count_table[self] = old_value + 1

	-- Don't double count references to children
	if old_value > 0 then return end
	if self.expr_type == "vector" then
		self.x:add_ref_counts(count_table)
		self.y:add_ref_counts(count_table)
		self.z:add_ref_counts(count_table)
	elseif self.expr_type == "unary_operation" then
		self.arg:add_ref_counts(count_table)
	elseif self.expr_type == "binary_operation" then
		self.arg1:add_ref_counts(count_table)
		self.arg2:add_ref_counts(count_table)
	elseif self.expr_type == "index" then
		self.vector:add_ref_counts(count_table)
	end
end

-- Get a table of how many times each expression is referenced
local function get_ref_counts(expr_list)
	local count_table = {}
	for _, expr in ipairs(expr_list) do
		expr:add_ref_counts(count_table)
	end
	return count_table
end

-- Takes a list of expressions and returns a mapping from expressions to variable
-- indices. Expressions without a mapping are not stored in a variable.
local function get_variable_map(expr_list)
	local count_table = get_ref_counts(expr_list)
	local variable_map = {}
	local next_var = 0
	for expr, count in pairs(count_table) do
		if count > 1 then
			variable_map[expr] = next_var
			next_var = next_var + 1
		end
	end

	return variable_map, next_var
end

-- Prints a trace of what the output bytecode would do.
function expr_meta:trace(variable_map, visited)
	local self_index = variable_map[self]
	if self_index and visited[self] then
		-- Value already calculated
		print("Push variable", self_index)
		return
	elseif self.expr_type == "constant_scalar" then
		print("Push constant", self.value)
	elseif self.expr_type == "vector" then
		self.x:trace(variable_map, visited)
		self.y:trace(variable_map, visited)
		self.z:trace(variable_map, visited)
		print("Pop 3 scalars and make a vector")
	elseif self.expr_type == "unary_operation" then
		self.arg:trace(variable_map, visited)
		print("Pop a value and do op", self.operation)
	elseif self.expr_type == "binary_operation" then
		self.arg1:trace(variable_map, visited)
		self.arg2:trace(variable_map, visited)
		print("Pop 2 values and do op", self.operation)
	elseif self.expr_type == "index" then
		self.vector:trace(variable_map, visited)
		print("Pop vector and push index", self.index)
	elseif self.expr_type == "time" then
		print("Push time")
	end

	if self_index then
		-- Store variable
		print("Copy top of stack to var", self_index)
		visited[self] = true
	end
end

-- Also update src/particles.h
local opcodes = {
	push_constant = 0,
	push_variable = 1,
	write_variable = 2,
	make_vector = 3,
	index_x = 4,
	index_y = 5,
	index_z = 6,
	push_time = 7,
	add = 8,
	subtract = 9,
	multiply = 10,
	divide = 11,
	power = 12,
	random = 13,
	lt = 14,
	le = 15,
	atan2 = 16,
	acos = 17,
	asin = 18,
	atan = 19,
	ceil = 20,
	cos = 21,
	floor = 22,
	log = 23,
	sin = 24,
	tan = 25,
}

local indices = {
	x = opcodes.index_x,
	y = opcodes.index_y,
	z = opcodes.index_z,
}

-- output is a table to append commands to.
function expr_meta:append_commands(variable_map, visited, output)
	local self_index = variable_map[self]
	if self_index and visited[self] then
		-- Value already calculated
		table.insert(output, {
			operation = opcodes.push_variable,
			self_index,
		})
		return
	elseif self.expr_type == "constant_scalar" then
		table.insert(output, {
			operation = opcodes.push_constant,
			self.value,
		})
	elseif self.expr_type == "vector" then
		self.x:append_commands(variable_map, visited, output)
		self.y:append_commands(variable_map, visited, output)
		self.z:append_commands(variable_map, visited, output)
		table.insert(output, {
			operation = opcodes.make_vector,
		})
	elseif self.expr_type == "unary_operation" then
		self.arg:append_commands(variable_map, visited, output)
		table.insert(output, {
			operation = assert(opcodes[self.operation])
		})
	elseif self.expr_type == "binary_operation" then
		self.arg1:append_commands(variable_map, visited, output)
		self.arg2:append_commands(variable_map, visited, output)
		table.insert(output, {
			operation = assert(opcodes[self.operation]),
		})
	elseif self.expr_type == "index" then
		self.vector:append_commands(variable_map, visited, output)
		table.insert(output, {
			operation = assert(indices[self.index]),
		})
	elseif self.expr_type == "time" then
		table.insert(output, {
			operation = opcodes.push_time,
		})
	else
		error ("Invalid expression type")
	end

	if self_index then
		-- Store variable
		table.insert(output, {
			operation = opcodes.write_variable,
			self_index,
		})
		visited[self] = true
	end
end

local function make_exprs(expr_likes)
	local exprs = {}
	for i, expr_like in ipairs(expr_likes) do
		exprs[i] = expression.expression(expr_like)
	end

	return exprs
end

-- Returns both a list of compiled expressions and the number of variables
-- needed.
function expression.compile(exprs)
	local actual_exprs = make_exprs(exprs)
	local v_map, num_vars = get_variable_map(actual_exprs)
	local out_codes = {}

	local visited = {}
	for i, actual_expr in ipairs(actual_exprs) do
		local out_code = {}
		actual_expr:append_commands(v_map, visited, out_code)
		out_codes[i] = out_code
	end

	return out_codes, num_vars
end

function expression.trace(exprs)
	local actual_exprs = make_exprs(exprs)
	local v_map = get_variable_map(actual_exprs)

	local visited = {}
	for i, actual_expr in ipairs(actual_exprs) do
		actual_expr:trace(v_map, visited)
	end
end

-- Implementations of things from the math library

local function unop_fun(op)
	return function(arg)
		return expression.unary_operation(op, arg)
	end
end

function expression.abs(x)
	pos = expression.gt(x, 0)
	return pos * x + (1 - pos) * x
end

expression.acos = unop_fun("acos")
expression.asin = unop_fun("asin")
expression.atan = unop_fun("atan")
expression.atan2 = binop_fun("atan2")
expression.ceil = unop_fun("ceil")
expression.cos = unop_fun("cos")

function expression.sinh(x)
	return 0.5 * (expression.exp(x) + expression.exp(-x))
end

function expression.exp(x)
	return math.exp(1)^x
end

expression.floor = unop_fun("floor")
expression.log = unop_fun("log")

function expression.max(x, y)
	xBigger = expression.gt(x, y)
	return xBigger * x + (1 - xBigger) * y
end

function expression.min(x, y)
	xBigger = expression.gt(x, y)
	return xBigger * y + (1 - xBigger) * x
end

function expression.pow(x, y)
	return x^y
end

expression.sin = unop_fun("sin")

function expression.sinh(x)
	return 0.5 * (expression.exp(x) - expression.exp(-x))
end

function expression.sqrt(x)
	return x^0.5
end

expression.tan = unop_fun("tan")

function expression.tanh(x)
	posExp = expression.exp(x)
	negExp = expression.exp(-x)

	return (posExp - negExp) / (posExp + negExp)
end
