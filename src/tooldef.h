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

#ifndef TOOLDEF_HEADER
#define TOOLDEF_HEADER

#include <string>
#include <iostream>

/*
	TODO: Rename to tooldef.h
*/

struct ToolDiggingProperties
{
	// time = basetime + sum(feature here * feature in MaterialProperties)
	float basetime;
	float dt_weight;
	float dt_crackiness;
	float dt_crumbliness;
	float dt_cuttability;
	float basedurability;
	float dd_weight;
	float dd_crackiness;
	float dd_crumbliness;
	float dd_cuttability;

	ToolDiggingProperties(
			float a=0.75, float b=0, float c=0, float d=0, float e=0,
			float f=50, float g=0, float h=0, float i=0, float j=0);
};

struct ToolDefinition
{
	std::string imagename;
	ToolDiggingProperties properties;

	ToolDefinition(){}
	ToolDefinition(const std::string &imagename_,
			ToolDiggingProperties properties_):
		imagename(imagename_),
		properties(properties_)
	{}
	
	std::string dump();
	void serialize(std::ostream &os);
	void deSerialize(std::istream &is);
};

class IToolDefManager
{
public:
	IToolDefManager(){}
	virtual ~IToolDefManager(){}
	virtual const ToolDefinition* getToolDefinition(const std::string &toolname) const=0;
	virtual std::string getImagename(const std::string &toolname) const =0;
	virtual ToolDiggingProperties getDiggingProperties(
			const std::string &toolname) const =0;
};

class IWritableToolDefManager : public IToolDefManager
{
public:
	IWritableToolDefManager(){}
	virtual ~IWritableToolDefManager(){}
	virtual const ToolDefinition* getToolDefinition(const std::string &toolname) const=0;
	virtual std::string getImagename(const std::string &toolname) const =0;
	virtual ToolDiggingProperties getDiggingProperties(
			const std::string &toolname) const =0;
			
	virtual bool registerTool(std::string toolname, const ToolDefinition &def)=0;
};

IWritableToolDefManager* createToolDefManager();

#endif

