/*
BlockPlanet
Copyright (C) 2012 MiJyn, Joel Leclerc <mijyn@mail.com>
Licensed under GPLv3


Based on:
Minetest-c55
Copyright (C) 2010-2011 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "settings.h"
#include "common_irrlicht.h"
#include <string>
#include <jthread.h>
#include <jmutex.h>
#include <jmutexautolock.h>
#include "strfnd.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "debug.h"
#include "utility.h"
#include "log.h"

void Settings::writeLines(std::ostream &os)
{
	JMutexAutoLock lock(m_mutex);

	for(core::map<std::string, std::string>::Iterator
			i = m_settings.getIterator();
			i.atEnd() == false; i++)
	{
		std::string name = i.getNode()->getKey();
		std::string value = i.getNode()->getValue();
		os<<name<<" = "<<value<<"\n";
	}
}

bool Settings::parseConfigLine(const std::string &line)
{
	JMutexAutoLock lock(m_mutex);

	std::string trimmedline = trim(line);
	
	// Ignore empty lines and comments
	if(trimmedline.size() == 0 || trimmedline[0] == '#')
	{
		return true;
	}
	//infostream<<"trimmedline=\""<<trimmedline<<"\""<<std::endl;
	Strfnd sf(trim(line));
	std::string name = sf.next("=");
	name = trim(name);
	if(name == "")
	{
		return true;
	}
	
	std::string value = sf.next("\n");
	value = trim(value);
	/*infostream<<"Config name=\""<<name<<"\" value=\""
		<<value<<"\""<<std::endl;*/
	
	m_settings[name] = value;
	
	return true;
}

void Settings::parseConfigLines(std::istream &is, const std::string &endstring)
{
	for(;;)
	{
		if(is.eof())
		{
			break;
		}
		std::string line;
		std::getline(is, line);
		std::string trimmedline = trim(line);
		if(endstring != "")
		{
			if(trimmedline == endstring)
			{
				break;
			}
		}
		parseConfigLine(line);
	}
}

/*
	Description: Parses a configuration object
	Returns: False on EOF
*/
bool Settings::parseConfigObject(std::istream &is)
{
	if(is.eof())
	{
		return false;
	}
	
	/*
		NOTE: This function might be expanded to allow multi-line
		      settings.
	*/
	std::string line;
	std::getline(is, line);
	//infostream<<"got line: \""<<line<<"\""<<std::endl;

	return parseConfigLine(line);
}

/*
	Description: Reads a configuration file
	Returns: True on success
*/
bool Settings::readConfigFile(const char *filename)
{
	std::ifstream is(filename);
	if(is.good() == false)
	{
		return false;
	}

	infostream << "Parsing configuration file: \"" <<filename << "\"" << std::endl;

	while(parseConfigObject(is));

	return true;
}

/*
	Description: Reads a configuration object from stream (usually a single line)
		and adds it to dst.
	
	Notes: Preserves comments and empty lines.
		Settings that were added to dst are also added to updated.
		key of updated is setting name, value of updated is dummy.

	Returns: False on EOF
*/
bool getUpdatedConfigObject(std::istream &is,
		core::list<std::string> &dst,
		core::map<std::string, bool> &updated,
		bool &value_changed)
{
	JMutexAutoLock lock(m_mutex);
	
	if(is.eof())
	{
		return false;
	}
	
	// NOTE: This function will be expanded to allow multi-line settings
	std::string line;
	std::getline(is, line);

	std::string trimmedline = trim(line);
	std::string line_end = "";

	if(is.eof() == false)
	{
		line_end = "\n";
	}
	
	// Ignore empty lines and comments
	if(trimmedline.size() == 0 || trimmedline[0] == '#')
	{
		dst.push_back(line+line_end);
		return true;
	}

	Strfnd sf(trim(line));

	std::string name = sf.next("=");
	name = trim(name);

	if(name == "")
	{
		dst.push_back(line+line_end);
		return true;
	}
	
	std::string value = sf.next("\n");
	value = trim(value);
	
	if(m_settings.find(name))
	{
		std::string newvalue = m_settings[name];
		
		if(newvalue != value)
		{
			infostream<<"Changing value of \""<<name<<"\" = \""
					<<value<<"\" -> \""<<newvalue<<"\""
					<<std::endl;
			value_changed = true;
		}

		dst.push_back(name + " = " + newvalue + line_end);

		updated[name] = true;
	}
	
	return true;
}

/*
	Description: Updates configuration file

	Returns: True on success
*/
bool updateConfigFile(const char *filename)
{
	infostream<<"Updating configuration file: \""
			<<filename<<"\""<<std::endl;

	core::list<std::string> objects;
	core::map<std::string, bool> updated;
	bool something_actually_changed = false;

	// Read and modify stuff
	{
		std::ifstream is(filename);
		if(is.good() == false)
		{
			infostream<<"updateConfigFile():"
					" Error opening configuration file"
					" for reading: \""
					<<filename<<"\""<<std::endl;
		}
		else
		{
			while(getUpdatedConfigObject(is, objects, updated,
					something_actually_changed));
		}
	}
	
	JMutexAutoLock lock(m_mutex);
	
	// If something not yet determined to have been changed, check if
	// any new stuff was added
	if(!something_actually_changed)
	{
		for(core::map<std::string, std::string>::Iterator
				i = m_settings.getIterator();
				i.atEnd() == false; i++)
		{
			if(updated.find(i.getNode()->getKey()))
			{
				continue;
			}
			something_actually_changed = true;
			break;
		}
	}
	
	// If nothing was actually changed, skip writing the file
	if(!something_actually_changed)
	{
		infostream<<"Skipping writing of "<<filename
				<<" because content wouldn't be modified"<<std::endl;
		return true;
	}
	
	// Write stuff back
	{
		std::ofstream os(filename);
		if(os.good() == false)
		{
			errorstream<<"Error opening configuration file"
					" for writing: \""
					<<filename<<"\""<<std::endl;
			return false;
		}
		
		/*
			Write updated stuff
		*/
		for(core::list<std::string>::Iterator
				i = objects.begin();
				i != objects.end(); i++)
		{
			os<<(*i);
		}

		/*
			Write stuff that was not already in the file
		*/
		for(core::map<std::string, std::string>::Iterator
				i = m_settings.getIterator();
				i.atEnd() == false; i++)
		{
			if(updated.find(i.getNode()->getKey()))
			{
				continue;
			}
			std::string name = i.getNode()->getKey();
			std::string value = i.getNode()->getValue();
			infostream<<"Adding \""<<name<<"\" = \""<<value<<"\""
					<<std::endl;
			os<<name<<" = "<<value<<"\n";
		}
	}
	
	return true;
}
