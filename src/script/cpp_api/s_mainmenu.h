/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "cpp_api/s_base.h"
#include "util/string.h"
#include "gui/guiMainMenu.h"

class ScriptApiMainMenu : virtual public ScriptApiBase {
public:
	/**
	 * Hand over MainMenuDataForScript to lua to inform lua of the content
	 * @param data the data
	 */
	void setMainMenuData(MainMenuDataForScript *data);

	/**
	 * process events received from formspec
	 * @param text events in textual form
	 */
	void handleMainMenuEvent(std::string text);

	/**
	 * process field data received from formspec
	 * @param fields data in field format
	 */
	void handleMainMenuButtons(const StringMap &fields);
};
