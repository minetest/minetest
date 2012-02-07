/*
Minetest-c55
Copyright (C) 2012 sapier

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/
#ifndef __COLLIDABLEOBJECT_H__
#define __COLLIDABLEOBJECT_H__

#include <irrlichttypes.h>

class CollidableObject {
public:
	virtual aabb3f* getCollisionBox() = 0;

	inline float getFlexibility() { return m_Flexibility; }
	inline int   getWeight()      { return m_Weight; }
protected:
	int m_Weight;
	float m_Flexibility;
	aabb3f m_collisionbox;
};

#endif //__COLLIDABLEOBJECT_H__
