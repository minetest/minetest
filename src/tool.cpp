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

#include "tool.h"

std::string tool_get_imagename(const std::string &toolname)
{
	if(toolname == "WPick")
		return "tool_woodpick.png";
	else if(toolname == "STPick")
		return "tool_stonepick.png";
	else if(toolname == "SteelPick")
		return "tool_steelpick.png";
	else if(toolname == "MesePick")
		return "tool_mesepick.png";
	else if(toolname == "WShovel")
		return "tool_woodshovel.png";
	else if(toolname == "STShovel")
		return "tool_stoneshovel.png";
	else if(toolname == "SteelShovel")
		return "tool_steelshovel.png";
	else if(toolname == "WAxe")
		return "tool_woodaxe.png";
	else if(toolname == "STAxe")
		return "tool_stoneaxe.png";
	else if(toolname == "SteelAxe")
		return "tool_steelaxe.png";
	else if(toolname == "WSword")
		return "tool_woodsword.png";
	else if(toolname == "STSword")
		return "tool_stonesword.png";
	else if(toolname == "SteelSword")
		return "tool_steelsword.png";
	else
		return "cloud.png";
}

ToolDiggingProperties tool_get_digging_properties(const std::string &toolname)
{
	// weight, crackiness, crumbleness, cuttability
	if(toolname == "WPick")
		return ToolDiggingProperties(2.0, 0,-1,2,0, 50, 0,0,0,0);
	else if(toolname == "STPick")
		return ToolDiggingProperties(1.5, 0,-1,2,0, 100, 0,0,0,0);
	else if(toolname == "SteelPick")
		return ToolDiggingProperties(1.0, 0,-1,2,0, 300, 0,0,0,0);

	else if(toolname == "MesePick")
		return ToolDiggingProperties(0, 0,0,0,0, 1337, 0,0,0,0);
	
	else if(toolname == "WShovel")
		return ToolDiggingProperties(2.0, 0.5,2,-1.5,0.3, 50, 0,0,0,0);
	else if(toolname == "STShovel")
		return ToolDiggingProperties(1.5, 0.5,2,-1.5,0.1, 100, 0,0,0,0);
	else if(toolname == "SteelShovel")
		return ToolDiggingProperties(1.0, 0.5,2,-1.5,0.0, 300, 0,0,0,0);

	// weight, crackiness, crumbleness, cuttability
	else if(toolname == "WAxe")
		return ToolDiggingProperties(2.0, 0.5,-0.2,1,-0.5, 50, 0,0,0,0);
	else if(toolname == "STAxe")
		return ToolDiggingProperties(1.5, 0.5,-0.2,1,-0.5, 100, 0,0,0,0);
	else if(toolname == "SteelAxe")
		return ToolDiggingProperties(1.0, 0.5,-0.2,1,-0.5, 300, 0,0,0,0);

	else if(toolname == "WSword")
		return ToolDiggingProperties(3.0, 3,0,1,-1, 50, 0,0,0,0);
	else if(toolname == "STSword")
		return ToolDiggingProperties(2.5, 3,0,1,-1, 100, 0,0,0,0);
	else if(toolname == "SteelSword")
		return ToolDiggingProperties(2.0, 3,0,1,-1, 300, 0,0,0,0);

	// Properties of hand
	return ToolDiggingProperties(0.5, 1,0,-1,0, 50, 0,0,0,0);
}


