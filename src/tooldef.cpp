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

#include "tooldef.h"
#include "irrlichttypes.h"
#include "log.h"
#include <ostream>

class CToolDefManager: public IWritableToolDefManager
{
public:
	virtual ~CToolDefManager()
	{
		for(core::map<std::string, ToolDefinition*>::Iterator
				i = m_tool_definitions.getIterator();
				i.atEnd() == false; i++){
			delete i.getNode()->getValue();
		}
	}
	virtual const ToolDefinition* getToolDefinition(const std::string &toolname) const
	{
		core::map<std::string, ToolDefinition*>::Node *n;
		n = m_tool_definitions.find(toolname);
		if(n == NULL)
			return NULL;
		return n->getValue();
	}
	virtual std::string getImagename(const std::string &toolname) const
	{
		const ToolDefinition *def = getToolDefinition(toolname);
		if(def == NULL)
			return "";
		return def->imagename;
	}
	virtual ToolDiggingProperties getDiggingProperties(
			const std::string &toolname) const
	{
		const ToolDefinition *def = getToolDefinition(toolname);
		// If tool does not exist, just return an impossible
		if(def == NULL){
			// If tool does not exist, try empty name
			const ToolDefinition *def = getToolDefinition("");
			if(def == NULL) // If that doesn't exist either, return default
				return ToolDiggingProperties();
			return def->properties;
		}
		return def->properties;
	}
	virtual bool registerTool(std::string toolname, const ToolDefinition &def)
	{
		infostream<<"registerTool: registering tool \""<<toolname<<"\""<<std::endl;
		core::map<std::string, ToolDefinition*>::Node *n;
		n = m_tool_definitions.find(toolname);
		if(n != NULL){
			errorstream<<"registerTool: registering tool \""<<toolname
					<<"\" failed: name is already registered"<<std::endl;
			return false;
		}
		m_tool_definitions[toolname] = new ToolDefinition(def);
		return true;
	}
private:
	// Key is name
	core::map<std::string, ToolDefinition*> m_tool_definitions;
};

IWritableToolDefManager* createToolDefManager()
{
	return new CToolDefManager();
}

