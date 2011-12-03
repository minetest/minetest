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
#include <sstream>
#include "utility.h"
#include <map>

ToolDiggingProperties::ToolDiggingProperties(float full_punch_interval_,
		float a, float b, float c, float d, float e,
		float f, float g, float h, float i, float j):
	full_punch_interval(full_punch_interval_),
	basetime(a),
	dt_weight(b),
	dt_crackiness(c),
	dt_crumbliness(d),
	dt_cuttability(e),
	basedurability(f),
	dd_weight(g),
	dd_crackiness(h),
	dd_crumbliness(i),
	dd_cuttability(j)
{}

std::string ToolDefinition::dump()
{
	std::ostringstream os(std::ios::binary);
	os<<"[ToolDefinition::dump() not implemented due to lazyness]"
			<<std::endl;
	return os.str();
}

void ToolDefinition::serialize(std::ostream &os)
{
	writeU8(os, 0); // version
	os<<serializeString(imagename);
	writeF1000(os, properties.basetime);
	writeF1000(os, properties.dt_weight);
	writeF1000(os, properties.dt_crackiness);
	writeF1000(os, properties.dt_crumbliness);
	writeF1000(os, properties.dt_cuttability);
	writeF1000(os, properties.basedurability);
	writeF1000(os, properties.dd_weight);
	writeF1000(os, properties.dd_crackiness);
	writeF1000(os, properties.dd_crumbliness);
	writeF1000(os, properties.dd_cuttability);
	writeF1000(os, properties.full_punch_interval);
}

void ToolDefinition::deSerialize(std::istream &is)
{
	int version = readU8(is);
	if(version != 0) throw SerializationError(
			"unsupported ToolDefinition version");
	imagename = deSerializeString(is);
	properties.basetime = readF1000(is);
	properties.dt_weight = readF1000(is);
	properties.dt_crackiness = readF1000(is);
	properties.dt_crumbliness = readF1000(is);
	properties.dt_cuttability = readF1000(is);
	properties.basedurability = readF1000(is);
	properties.dd_weight = readF1000(is);
	properties.dd_crackiness = readF1000(is);
	properties.dd_crumbliness = readF1000(is);
	properties.dd_cuttability = readF1000(is);
	try{
		properties.full_punch_interval = readF1000(is);
	}catch(SerializationError &e){} // Temporary for 0.4.dev
}

class CToolDefManager: public IWritableToolDefManager
{
public:
	virtual ~CToolDefManager()
	{
		clear();
	}
	virtual const ToolDefinition* getToolDefinition(const std::string &toolname_) const
	{
		// Convert name according to possible alias
		std::string toolname = toolname_;
		std::map<std::string, std::string>::const_iterator i;
		i = m_aliases.find(toolname);
		if(i != m_aliases.end()){
			toolname = i->second;
		}
		// Get the definition
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
		m_tool_definitions[toolname] = new ToolDefinition(def);

		// Remove conflicting alias if it exists
		bool alias_removed = (m_aliases.erase(toolname) != 0);
		if(alias_removed)
			infostream<<"tdef: erased alias "<<toolname
					<<" because tool was defined"<<std::endl;
		
		return true;
	}
	virtual void clear()
	{
		for(core::map<std::string, ToolDefinition*>::Iterator
				i = m_tool_definitions.getIterator();
				i.atEnd() == false; i++){
			delete i.getNode()->getValue();
		}
		m_tool_definitions.clear();
		m_aliases.clear();
	}
	virtual void setAlias(const std::string &name,
			const std::string &convert_to)
	{
		if(getToolDefinition(name) != NULL){
			infostream<<"tdef: not setting alias "<<name<<" -> "<<convert_to
					<<": "<<name<<" is already defined"<<std::endl;
			return;
		}
		infostream<<"tdef: setting alias "<<name<<" -> "<<convert_to
				<<std::endl;
		m_aliases[name] = convert_to;
	}
	virtual void serialize(std::ostream &os)
	{
		writeU8(os, 0); // version
		u16 count = m_tool_definitions.size();
		writeU16(os, count);
		for(core::map<std::string, ToolDefinition*>::Iterator
				i = m_tool_definitions.getIterator();
				i.atEnd() == false; i++){
			std::string name = i.getNode()->getKey();
			ToolDefinition *def = i.getNode()->getValue();
			// Serialize name
			os<<serializeString(name);
			// Serialize ToolDefinition and write wrapped in a string
			std::ostringstream tmp_os(std::ios::binary);
			def->serialize(tmp_os);
			os<<serializeString(tmp_os.str());
		}

		writeU16(os, m_aliases.size());
		for(std::map<std::string, std::string>::const_iterator
				i = m_aliases.begin(); i != m_aliases.end(); i++)
		{
			os<<serializeString(i->first);
			os<<serializeString(i->second);
		}
	}
	virtual void deSerialize(std::istream &is)
	{
		// Clear everything
		clear();
		// Deserialize
		int version = readU8(is);
		if(version != 0) throw SerializationError(
				"unsupported ToolDefManager version");
		u16 count = readU16(is);
		for(u16 i=0; i<count; i++){
			// Deserialize name
			std::string name = deSerializeString(is);
			// Deserialize a string and grab a ToolDefinition from it
			std::istringstream tmp_is(deSerializeString(is), std::ios::binary);
			ToolDefinition def;
			def.deSerialize(tmp_is);
			// Register
			registerTool(name, def);
		}

		u16 num_aliases = readU16(is);
		if(!is.eof()){
			for(u16 i=0; i<num_aliases; i++){
				std::string name = deSerializeString(is);
				std::string convert_to = deSerializeString(is);
				m_aliases[name] = convert_to;
			}
		}
	}
private:
	// Key is name
	core::map<std::string, ToolDefinition*> m_tool_definitions;
	// Aliases
	std::map<std::string, std::string> m_aliases;
};

IWritableToolDefManager* createToolDefManager()
{
	return new CToolDefManager();
}

