/*
Minetest
Copyright (C) 2010-2018 nerzhul, Loic BLOT <loic.blot@unix-experience.fr>

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

#include <functional>
#include <vector>
#include "../activeobjectmgr.h"
#include "clientobject.h"

namespace client
{
class ActiveObjectMgr : public ::ActiveObjectMgr<ClientActiveObject>
{
public:
	void clear();
	void step(float dtime,
			const std::function<void(ClientActiveObject *)> &f) override;
	bool registerObject(ClientActiveObject *obj) override;
	void removeObject(u16 id) override;

	void getActiveObjects(const v3f &origin, f32 max_d,
			std::vector<DistanceSortedActiveObject> &dest);
	// Similar to above, but takes selection box sizes, and line direction into
	// account.
	// Objects without selectionbox are not returned.
	// Returned distances are in direction of shootline.
	// Distance check is coarse.
	void getActiveSelectableObjects(const core::line3d<f32> &shootline,
			std::vector<DistanceSortedActiveObject> &dest);
};
} // namespace client
