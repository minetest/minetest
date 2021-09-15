/*
Minetest
Copyright (C) 2021 Jude Melton-Houghton

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

#pragma once

/**
 * A scoped setting of a reference. The reference is set to the new value when
 * the SetInScope is constructed. On destruction the reference is reset to the
 * value it had before construction.
 *
 * @tparam T The referenced type
 */
template<typename T>
class SetInScope
{
public:
	SetInScope(T &ref, const T &&val): m_ref(ref), m_old_val(ref) { ref = val; }

	~SetInScope() { m_ref = m_old_val; }

private:
	T &m_ref;
	T m_old_val;
};
