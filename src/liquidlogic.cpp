/*
Minetest
Copyright (C) 2019 Pierre-Yves Rollo <dev@pyrollo.com>

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

#include "liquidlogic.h"
#include "map.h"
#include "nodedef.h"

LiquidLogic::LiquidLogic(Map *map, IGameDef *gamedef) :
	m_map(map),
	m_gamedef(gamedef)
{
	m_ndef = m_map->getNodeDefManager();
}
