-- LuaLinq - http://code.google.com/p/lualinq/
-- ------------------------------------------------------------------------
-- Copyright (c) 2012, Marco Mastropaolo (Xanathar)
-- All rights reserved.
-- 
-- Redistribution and use in source and binary forms, with or without modification, 
-- are permitted provided that the following conditions are met:
-- 
--  o Redistributions of source code must retain the above copyright notice, 
-- 	  this list of conditions and the following disclaimer.
--  o Redistributions in binary form must reproduce the above copyright notice, 
-- 	  this list of conditions and the following disclaimer in the documentation 
-- 	  and/or other materials provided with the distribution.
--  o Neither the name of Marco Mastropaolo nor the names of its contributors 
-- 	  may be used to endorse or promote products derived from this software 
-- 	  without specific prior written permission.
-- 
-- THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND 
-- ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
-- WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
-- IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
-- INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
-- BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
-- DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
-- LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
-- OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
-- OF THE POSSIBILITY OF SUCH DAMAGE.
-- ------------------------------------------------------------------------

-- how much log information is printed: 3 => verbose, 2 => info, 1 => only warning and errors, 0 => only errors, -1 => silent
LOG_LEVEL = 1

-- prefix for the printed logs
LOG_PREFIX = "LuaLinq: "





-- ============================================================
-- DEBUG TRACER
-- ============================================================

LIB_VERSION_TEXT = "1.5.2"
LIB_VERSION = 152

function setLogLevel(level)
	LOG_LEVEL = level;
end

function _log(level, prefix, text)
	if (level <= LOG_LEVEL) then
		print(prefix .. LOG_PREFIX .. text)
	end
end

function logq(self, method)
	if (LOG_LEVEL >= 3) then
		logv("after " .. method .. " => " .. #self.m_Data .. " items : " .. _dumpData(self))
	end
end

function _dumpData(self)
	local items = #self.m_Data
	local dumpdata = "q{ "
	
	for i = 1, 3 do
		if (i <= items) then
			if (i ~= 1) then
				dumpdata = dumpdata .. ", "
			end
			dumpdata = dumpdata .. tostring(self.m_Data[i])
		end
	end
	
	if (items > 3) then
		dumpdata = dumpdata .. ", ..." .. items .. " }"
	else
		dumpdata = dumpdata .. " }"
	end

	return dumpdata
end



function logv(txt)
	_log(3, "[..] ", txt)
end

function logi(txt)
	_log(2, "[ii] ", txt)
end

function logw(txt)
	_log(1, "[W?] ", txt)
end

function loge(txt)
	_log(0, "[E!] ", txt)
end


-- ============================================================
-- CONSTRUCTOR
-- ============================================================

-- [private] Creates a linq data structure from an array without copying the data for efficiency
function _new_lualinq(method, collection)
	local self = { }
	
	self.classid_71cd970f_a742_4316_938d_1998df001335 = 2
	
	self.m_Data = collection
	
	self.concat = _concat
	self.select = _select
	self.selectMany = _selectMany
	self.where = _where
	self.whereIndex = _whereIndex
	self.take = _take
	self.skip = _skip
	self.zip = _zip
	
	self.distinct = _distinct 
	self.union = _union
	self.except = _except
	self.intersection = _intersection
	self.exceptby = _exceptby
	self.intersectionby = _intersectionby
	self.exceptBy = _exceptby
	self.intersectionBy = _intersectionby

	self.first = _first
	self.last = _last
	self.min = _min
	self.max = _max
	self.random = _random

	self.any = _any
	self.all = _all
	self.contains = _contains

	self.count = _count
	self.sum = _sum
	self.average = _average

	self.dump = _dump
	
	self.map = _map
	self.foreach = _foreach
	self.xmap = _xmap

	self.toArray = _toArray
	self.toDictionary = _toDictionary
	self.toIterator = _toIterator
	self.toTuple = _toTuple

	-- shortcuts
	self.each = _foreach
	self.intersect = _intersection
	self.intersectby = _intersectionby
	self.intersectBy = _intersectionby
	
	
	logq(self, "from")

	return self
end
-- ============================================================
-- GENERATORS
-- ============================================================

-- Tries to autodetect input type and uses the appropriate from method
function from(auto)
	if (auto == nil) then
		return fromNothing()
	elseif (type(auto) == "function") then
		return fromIterator(auto)
	elseif (type(auto) == "table") then
		if (auto["classid_71cd970f_a742_4316_938d_1998df001335"] ~= nil) then
			return auto
		elseif (auto[1] == nil) then
			return fromDictionary(auto)
		elseif (type(auto[1]) == "function") then
			return fromIteratorsArray(auto)
		else
			return fromArrayInstance(auto)
		end
	end
	return fromNothing()
end

-- Creates a linq data structure from an array without copying the data for efficiency
function fromArrayInstance(collection)
	return _new_lualinq("fromArrayInstance", collection)
end

-- Creates a linq data structure from an array copying the data first (so that changes in the original
-- table do not reflect here)
function fromArray(array)
	local collection = { }
	for k,v in ipairs(array) do
		table.insert(collection, v)
	end
	return _new_lualinq("fromArray", collection)
end

-- Creates a linq data structure from a dictionary (table with non-consecutive-integer keys)
function fromDictionary(dictionary)
	local collection = { }
	
	for k,v in pairs(dictionary) do
		local kvp = {}
		kvp.key = k
		kvp.value = v
		
		table.insert(collection, kvp)
	end
	
	return _new_lualinq("fromDictionary", collection)
end

-- Creates a linq data structure from an iterator returning single items
function fromIterator(iterator)
	local collection = { }
	
	for s in iterator do
		table.insert(collection, s)
	end
	
	return _new_lualinq("fromIterator", collection)
end

-- Creates a linq data structure from an array of iterators each returning single items
function fromIteratorsArray(iteratorArray)
	local collection = { }

	for _, iterator in ipairs(iteratorArray) do
		for s in iterator do
			table.insert(collection, s)
		end
	end
	
	return _new_lualinq("fromIteratorsArray", collection)
end

-- Creates a linq data structure from a table of keys, values ignored
function fromSet(set)
	local collection = { }

	for k,v in pairs(set) do
		table.insert(collection, k)
	end
	
	return _new_lualinq("fromIteratorsArray", collection)
end


-- Creates an empty linq data structure
function fromNothing()
	return _new_lualinq("fromNothing", { } )
end

-- ============================================================
-- QUERY METHODS
-- ============================================================

-- Concatenates two collections together
function _concat(self, otherlinq)
	local result = { }

	for idx, value in ipairs(self.m_Data) do
		table.insert(result, value)
	end
	for idx, value in ipairs(otherlinq.m_Data) do
		table.insert(result, value)
	end
	
	return _new_lualinq(":concat", result)
end

-- Replaces items with those returned by the selector function or properties with name selector
function _select(self, selector)
	local result = { }

	if (type(selector) == "function") then
		for idx, value in ipairs(self.m_Data) do
			local newvalue = selector(value)
			if (newvalue ~= nil) then
				table.insert(result, newvalue)
			end
		end
	elseif (type(selector) == "string") then
		for idx, value in ipairs(self.m_Data) do
			local newvalue = value[selector]
			if (newvalue ~= nil) then
				table.insert(result, newvalue)
			end
		end
	else
		loge("select called with unknown predicate type");
	end	
	return _new_lualinq(":select", result)
end


-- Replaces items with those contained in arrays returned by the selector function
function _selectMany(self, selector)
	local result = { }

	for idx, value in ipairs(self.m_Data) do
		local newvalue = selector(value)
		if (newvalue ~= nil) then
			for ii, vv in ipairs(newvalue) do
				if (vv ~= nil) then
					table.insert(result, vv)
				end
			end
		end
	end
	
	return _new_lualinq(":selectMany", result)
end


-- Returns a linq data structure where only items for whose the predicate has returned true are included
function _where(self, predicate, refvalue, ...)
	local result = { }

	if (type(predicate) == "function") then
		for idx, value in ipairs(self.m_Data) do
			if (predicate(value, refvalue, from({...}):toTuple())) then
				table.insert(result, value)
			end
		end	
	elseif (type(predicate) == "string") then
		local refvals = {...}
		
		if (#refvals > 0) then
			table.insert(refvals, refvalue);
			return _intersectionby(self, predicate, refvals);
		elseif (refvalue ~= nil) then
			for idx, value in ipairs(self.m_Data) do
				if (value[predicate] == refvalue) then
					table.insert(result, value)
				end
			end	
		else
			for idx, value in ipairs(self.m_Data) do
				if (value[predicate] ~= nil) then
					table.insert(result, value)
				end
			end	
		end
	else
		loge("where called with unknown predicate type");
	end
	
	return _new_lualinq(":where", result)
end




-- Returns a linq data structure where only items for whose the predicate has returned true are included, indexed version
function _whereIndex(self, predicate)
	local result = { }

	for idx, value in ipairs(self.m_Data) do
		if (predicate(idx, value)) then
			table.insert(result, value)
		end
	end	
	
	return _new_lualinq(":whereIndex", result)
end

-- Return a linq data structure with at most the first howmany elements
function _take(self, howmany)
	return self:whereIndex(function(i, v) return i <= howmany; end)
end

-- Return a linq data structure skipping the first howmany elements
function _skip(self, howmany)
	return self:whereIndex(function(i, v) return i > howmany; end)
end

-- Zips two collections together, using the specified join function
function _zip(self, otherlinq, joiner)
	otherlinq = from(otherlinq) 

	local thismax = #self.m_Data
	local thatmax = #otherlinq.m_Data
	local result = {}
	
	if (thatmax < thismax) then thismax = thatmax; end
	
	for i = 1, thismax do
		result[i] = joiner(self.m_Data[i], otherlinq.m_Data[i]);
	end
	
	return _new_lualinq(":zip", result)
end

-- Returns only distinct items, using an optional comparator
function _distinct(self, comparator)
	local result = {}
	comparator = comparator or function (v1, v2) return v1 == v2; end
	
	for idx, value in ipairs(self.m_Data) do
		local found = false

		for _, value2 in ipairs(result) do
			if (comparator(value, value2)) then
				found = true
			end
		end
	
		if (not found) then
			table.insert(result, value)
		end
	end
	
	return _new_lualinq(":distinct", result)
end

-- Returns the union of two collections, using an optional comparator
function _union(self, other, comparator)
	return self:concat(from(other)):distinct(comparator)
end

-- Returns the difference of two collections, using an optional comparator
function _except(self, other, comparator)
	other = from(other)
	return self:where(function (v) return not other:contains(v, comparator) end)
end

-- Returns the intersection of two collections, using an optional comparator
function _intersection(self, other, comparator)
	other = from(other)
	return self:where(function (v) return other:contains(v, comparator) end)
end

-- Returns the difference of two collections, using a property accessor
function _exceptby(self, property, other)
	other = from(other)
	return self:where(function (v) return not other:contains(v[property]) end)
end

-- Returns the intersection of two collections, using a property accessor
function _intersectionby(self, property, other)
	other = from(other)
	return self:where(function (v) return other:contains(v[property]) end)
end

-- ============================================================
-- CONVERSION METHODS
-- ============================================================

-- Converts the collection to an array
function _toIterator(self)
	local i = 0
	local n = #self.m_Data
	return function ()
			i = i + 1
			if i <= n then return self.m_Data[i] end
		end
end

-- Converts the collection to an array
function _toArray(self)
	return self.m_Data
end

-- Converts the collection to a table using a selector functions which returns key and value for each item
function _toDictionary(self, keyValueSelector)
	local result = { }

	for idx, value in ipairs(self.m_Data) do
		local key, value = keyValueSelector(value)
		if (key ~= nil) then
			result[key] = value
		end
	end
	
	return result
end

-- Converts the lualinq struct to a tuple
function _toTuple(self)
	return unpack(self.m_Data)
end




-- ============================================================
-- TERMINATING METHODS
-- ============================================================

-- Return the first item or default if no items in the colelction
function _first(self, default)
	if (#self.m_Data > 0) then
		return self.m_Data[1]
	else
		return default
	end
end

-- Return the last item or default if no items in the colelction
function _last(self, default)
	if (#self.m_Data > 0) then
		return self.m_Data[#self.m_Data]
	else
		return default
	end
end

-- Returns true if any item satisfies the predicate. If predicate is null, it returns true if the collection has at least one item.
function _any(self, predicate)
	if (predicate == nil) then return #self.m_Data > 0; end

	for idx, value in ipairs(self.m_Data) do
		if (predicate(value)) then
			return true
		end
	end
	
	return false
end

-- Returns true if all items satisfy the predicate. If predicate is null, it returns true if the collection is empty.
function _all(self, predicate)
	if (predicate == nil) then return #self.m_Data == 0; end

	for idx, value in ipairs(self.m_Data) do
		if (not predicate(value)) then
			return false
		end
	end
	
	return true
end

-- Returns the number of items satisfying the predicate. If predicate is null, it returns the number of items in the collection.
function _count(self, predicate)
	if (predicate == nil) then return #self.m_Data; end

	local result = 0

	for idx, value in ipairs(self.m_Data) do
		if (predicate(value)) then
			result = result + 1
		end
	end
	
	return result
end


-- Prints debug data.
function _dump(self)
	print(_dumpData(self));
end

-- Returns a random item in the collection, or default if no items are present
function _random(self, default)
	if (#self.m_Data == 0) then return default; end
	return self.m_Data[math.random(1, #self.m_Data)]
end

-- Returns true if the collection contains the specified item
function _contains(self, item, comparator)
	comparator = comparator or function (v1, v2) return v1 == v2; end
	for idx, value in ipairs(self.m_Data) do
		if (comparator(value, item)) then return true; end
	end
	return false
end


-- Calls the action for each item in the collection. Action takes 1 parameter: the item value.
-- If the action is a string, it calls that method with the additional parameters
function _foreach(self, action, ...)
	if (type(action) == "function") then
		for idx, value in ipairs(self.m_Data) do
			action(value, from({...}):toTuple())
		end
	elseif (type(action) == "string") then
		for idx, value in ipairs(self.m_Data) do
			value[action](value, from({...}):toTuple())
		end
	else
		loge("foreach called with unknown action type");
	end

	
	return self
end

-- Calls the accumulator for each item in the collection. Accumulator takes 2 parameters: value and the previous result of 
-- the accumulator itself (firstvalue for the first call) and returns a new result.
function _map(self, accumulator, firstvalue)
	local result = firstvalue

	for idx, value in ipairs(self.m_Data) do
		result = accumulator(value, result)
	end
	
	return result
end

-- Calls the accumulator for each item in the collection. Accumulator takes 3 parameters: value, the previous result of 
-- the accumulator itself (nil on first call) and the previous associated-result of the accumulator(firstvalue for the first call) 
-- and returns a new result and a new associated-result.
function _xmap(self, accumulator, firstvalue)
	local result = nil
	local lastval = firstvalue

	for idx, value in ipairs(self.m_Data) do
		result, lastval = accumulator(value, result, lastval)
	end
	
	return result
end

-- Returns the max of a collection. Selector is called with values and should return a number. Can be nil if collection is of numbers.
function _max(self, selector)
 	if (selector == nil) then 
		selector = function(n) return n; end
	end
  	return self:xmap(function(v, r, l) local res = selector(v); if (l == nil or res > l) then return v, res; else return r, l; end; end, nil)
end

-- Returns the min of a collection. Selector is called with values and should return a number. Can be nil if collection is of numbers.
function _min(self, selector)
	if (selector == nil) then 
		selector = function(n) return n; end
	end
  	return self:xmap(function(v, r, l) local res = selector(v); if (l == nil or res < l) then return v, res; else return r, l; end; end, nil)
end

-- Returns the sum of a collection. Selector is called with values and should return a number. Can be nil if collection is of numbers.
function _sum(self, selector)
	if (selector == nil) then 
		selector = function(n) return n; end
	end
	return self:map(function(n, r) r = r + selector(n); return r; end, 0)
end

-- Returns the average of a collection. Selector is called with values and should return a number. Can be nil if collection is of numbers.
function _average(self, selector)
	local count = self:count()
	if (count > 0) then
		return self:sum(selector) / count
	else
		return 0
	end
end

