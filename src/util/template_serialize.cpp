/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "template_serialize.h"

void BinaryKeyValueList::serialize(std::ostream &os)
{
	// Write version
	writeU8(os, 1);
	/* Write data */
	// Write number of values
	writeU32(os, m_values.size());
	for(std::map<u32, std::vector<Value> >::iterator
			it = m_values.begin(); it != m_values.end(); ++it)
	{
		u32 key = it->first;
		std::vector<Value> &list = it->second;
		bool count_written = false;
		/* Write key */
		if(key < 126){
			// Key is small enough; write in one byte
			u8 v = key;
			// If value count is 1, set high bit
			if(list.size() == 1){
				v |= 0x80;
				count_written = true;
			}
			writeU8(os, v);
		} else if(key < 65536){
			// Key is 16-bit; write 126|highbit
			u8 v = 126;
			// If value count is 1, set high bit
			if(list.size() == 1){
				v |= 0x80;
				count_written = true;
			}
			writeU8(os, v);
			// Write 16-bit key
			writeU16(os, key);
		} else{
			// Key is 32-bit; write 127|highbit
			u8 v = 127;
			// If value count is 1, set high bit
			if(list.size() == 1){
				v |= 0x80;
				count_written = true;
			}
			writeU8(os, v);
			// Write 32-bit key
			writeU32(os, key);
		}
		/* Write values */
		// Write value count if highbit optimization wasn't used
		if(!count_written){
			// If size is small, write directly; otherwise 255, size
			if(list.size() < 255){
				writeU8(os, list.size());
			} else{
				writeU8(os, 255);
				writeU32(os, list.size());
			}
		}
		for(size_t i=0; i<list.size(); i++){
			bool size_written = false;
			/* Write type */
			u8 t = list[i].t;
			assert(t < 0x80);
			// If size of value is 1, set highbit of type
			if(list[i].v.size() == 1){
				t |= 0x80;
				size_written = true;
			}
			writeU8(os, t);
			// Write size of value if highbit optimization wasn't used
			if(!size_written){
				// If size is small, write directly; otherwise 255, size
				if(list[i].v.size() < 255){
					writeU8(os, list[i].v.size());
				} else{
					writeU8(os, 255);
					writeU32(os, list[i].v.size());
				}
			}
			/* Write value */
			os.write((const char*)&list[i].v[0], list[i].v.size());
		}
	}
}
void BinaryKeyValueList::deSerialize(std::istream &is)
{
	m_values.clear();
	// Read version
	u8 version = readU8(is);
	if(version != 1)
		throw SerializationError("Invalid BinaryKeyValueList version");
	// Read data
	u32 num_keys = readU32(is);
	for(u32 i=0; i<num_keys; i++){
		if(!is.good())
			throw SerializationError("BinaryKeyValueList::deSerialize(): "
					"Truncated input");
		/* Read key */
		u8 key_v = readU8(is);
		bool single_value = !!(key_v & 0x80);
		u32 key = key_v & 0x7f;
		if(key == 126)
			key = readU16(is);
		else if(key == 127)
			key = readU32(is);
		/* Get list to deserialize into */
		std::map<u32, std::vector<Value> >::iterator it = m_values.find(key);
		if(it == m_values.end()){
			m_values[key] = std::vector<Value>();
			it = m_values.find(key);
		}
		std::vector<Value> &list = it->second;
		/* Read values */
		u32 num_values = 1;
		if(!single_value){
			num_values = readU8(is);
			if(num_values == 255)
				num_values = readU32(is);
		}
		for(u32 i=0; i<num_values; i++){
			u8 value_type_v = readU8(is);
			u8 value_type = value_type_v & 0x7f;
			bool single_byte = value_type_v & 0x80;
			u32 value_size = 1;
			if(!single_byte){
				value_size = readU8(is);
				if(value_size == 255)
					value_size = readU32(is);
			}
			Value value(value_type, value_size);
			is.read((char*)&value.v[0], value_size);
			list.push_back(value);
		}
	}
}

