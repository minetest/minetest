/*
Minetest-c55
Copyright (C) 2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "content_tool.h"
#include "tool.h"

void content_tool_init(IToolDefManager *mgr)
{
	mgr->registerTool("WPick",
			ToolDefinition("tool_woodpick.png",
			ToolDiggingProperties(2.0, 0,-1,2,0, 50, 0,0,0,0)));
	mgr->registerTool("STPick",
			ToolDefinition("tool_stonepick.png",
			ToolDiggingProperties(1.5, 0,-1,2,0, 100, 0,0,0,0)));
	mgr->registerTool("SteelPick",
			ToolDefinition("tool_steelpick.png",
			ToolDiggingProperties(1.0, 0,-1,2,0, 300, 0,0,0,0)));
	mgr->registerTool("MesePick",
			ToolDefinition("tool_mesepick.png",
			ToolDiggingProperties(0, 0,0,0,0, 1337, 0,0,0,0)));
	mgr->registerTool("WShovel",
			ToolDefinition("tool_woodshovel.png",
			ToolDiggingProperties(2.0, 0.5,2,-1.5,0.3, 50, 0,0,0,0)));
	mgr->registerTool("STShovel",
			ToolDefinition("tool_stoneshovel.png",
			ToolDiggingProperties(1.5, 0.5,2,-1.5,0.1, 100, 0,0,0,0)));
	mgr->registerTool("SteelShovel",
			ToolDefinition("tool_steelshovel.png",
			ToolDiggingProperties(1.0, 0.5,2,-1.5,0.0, 300, 0,0,0,0)));
	mgr->registerTool("WAxe",
			ToolDefinition("tool_woodaxe.png",
			ToolDiggingProperties(2.0, 0.5,-0.2,1,-0.5, 50, 0,0,0,0)));
	mgr->registerTool("STAxe",
			ToolDefinition("tool_stoneaxe.png",
			ToolDiggingProperties(1.5, 0.5,-0.2,1,-0.5, 100, 0,0,0,0)));
	mgr->registerTool("SteelAxe",
			ToolDefinition("tool_steelaxe.png",
			ToolDiggingProperties(1.0, 0.5,-0.2,1,-0.5, 300, 0,0,0,0)));
	mgr->registerTool("WSword",
			ToolDefinition("tool_woodsword.png",
			ToolDiggingProperties(3.0, 3,0,1,-1, 50, 0,0,0,0)));
	mgr->registerTool("STSword",
			ToolDefinition("tool_stonesword.png",
			ToolDiggingProperties(2.5, 3,0,1,-1, 100, 0,0,0,0)));
	mgr->registerTool("SteelSword",
			ToolDefinition("tool_steelsword.png",
			ToolDiggingProperties(2.0, 3,0,1,-1, 300, 0,0,0,0)));
	mgr->registerTool("",
			ToolDefinition("tool_hand.png",
			ToolDiggingProperties(0.5, 1,0,-1,0, 50, 0,0,0,0)));
}

