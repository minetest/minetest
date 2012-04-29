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

#ifndef SETTINGS_HEADER
#define SETTINGS_HEADER

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

enum ValueType
{
	VALUETYPE_STRING,
	VALUETYPE_FLAG // Doesn't take any arguments
};

struct ValueSpec
{
	ValueSpec(ValueType a_type, const char *a_help=NULL)
	{
		type = a_type;
		help = a_help;
	}
	ValueType type;
	const char *help;
};

class Settings
{
public:
	Settings()
	{
		m_mutex.Init();
	}

	void writeLines(std::ostream &os);

	bool parseConfigLine(const std::string &line);

	void parseConfigLines(std::istream &is, const std::string &endstring);

	bool parseConfigObject(std::istream &is);

	/*
		Description: Reads a configuration file
		Returns: True on success
	*/
	bool readConfigFile(const char *filename);

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
			bool &value_changed);

	/*
		Updates configuration file

		Returns true on success
	*/
	bool updateConfigFile(const char *filename);

	/*
		NOTE: Types of allowed_options are ignored

		returns true on success
	*/
	bool parseCommandLine(int argc, char *argv[],
			core::map<std::string, ValueSpec> &allowed_options)
	{
		int nonopt_index = 0;
		int i=1;
		for(;;)
		{
			if(i >= argc)
				break;
			std::string argname = argv[i];
			if(argname.substr(0, 2) != "--")
			{
				// If option doesn't start with -, read it in as nonoptX
				if(argname[0] != '-'){
					std::string name = "nonopt";
					name += itos(nonopt_index);
					set(name, argname);
					nonopt_index++;
					i++;
					continue;
				}
				errorstream<<"Invalid command-line parameter \""
						<<argname<<"\": --<option> expected."<<std::endl;
				return false;
			}
			i++;

			std::string name = argname.substr(2);

			core::map<std::string, ValueSpec>::Node *n;
			n = allowed_options.find(name);
			if(n == NULL)
			{
				errorstream<<"Unknown command-line parameter \""
						<<argname<<"\""<<std::endl;
				return false;
			}

			ValueType type = n->getValue().type;

			std::string value = "";
			
			if(type == VALUETYPE_FLAG)
			{
				value = "true";
			}
			else
			{
				if(i >= argc)
				{
					errorstream<<"Invalid command-line parameter \""
							<<name<<"\": missing value"<<std::endl;
					return false;
				}
				value = argv[i];
				i++;
			}
			

			infostream<<"Valid command-line parameter: \""
					<<name<<"\" = \""<<value<<"\""
					<<std::endl;
			set(name, value);
		}

		return true;
	}

	void set(std::string name, std::string value)
	{
		JMutexAutoLock lock(m_mutex);
		
		m_settings[name] = value;
	}

	void set(std::string name, const char *value)
	{
		JMutexAutoLock lock(m_mutex);

		m_settings[name] = value;
	}


	void setDefault(std::string name, std::string value)
	{
		JMutexAutoLock lock(m_mutex);
		
		m_defaults[name] = value;
	}

	bool exists(std::string name)
	{
		JMutexAutoLock lock(m_mutex);
		
		return (m_settings.find(name) || m_defaults.find(name));
	}

	std::string get(std::string name)
	{
		JMutexAutoLock lock(m_mutex);
		
		core::map<std::string, std::string>::Node *n;
		n = m_settings.find(name);
		if(n == NULL)
		{
			n = m_defaults.find(name);
			if(n == NULL)
			{
				throw SettingNotFoundException("Setting not found");
			}
		}

		return n->getValue();
	}

	bool getBool(std::string name)
	{
		return is_yes(get(name));
	}
	
	bool getFlag(std::string name)
	{
		try
		{
			return getBool(name);
		}
		catch(SettingNotFoundException &e)
		{
			return false;
		}
	}

	// Asks if empty
	bool getBoolAsk(std::string name, std::string question, bool def)
	{
		// If it is in settings
		if(exists(name))
			return getBool(name);
		
		std::string s;
		char templine[10];
		std::cout<<question<<" [y/N]: ";
		std::cin.getline(templine, 10);
		s = templine;

		if(s == "")
			return def;

		return is_yes(s);
	}

	float getFloat(std::string name)
	{
		return stof(get(name));
	}

	u16 getU16(std::string name)
	{
		return stoi(get(name), 0, 65535);
	}

	u16 getU16Ask(std::string name, std::string question, u16 def)
	{
		// If it is in settings
		if(exists(name))
			return getU16(name);
		
		std::string s;
		char templine[10];
		std::cout<<question<<" ["<<def<<"]: ";
		std::cin.getline(templine, 10);
		s = templine;

		if(s == "")
			return def;

		return stoi(s, 0, 65535);
	}

	s16 getS16(std::string name)
	{
		return stoi(get(name), -32768, 32767);
	}

	s32 getS32(std::string name)
	{
		return stoi(get(name));
	}

	v3f getV3F(std::string name)
	{
		v3f value;
		Strfnd f(get(name));
		f.next("(");
		value.X = stof(f.next(","));
		value.Y = stof(f.next(","));
		value.Z = stof(f.next(")"));
		return value;
	}

	v2f getV2F(std::string name)
	{
		v2f value;
		Strfnd f(get(name));
		f.next("(");
		value.X = stof(f.next(","));
		value.Y = stof(f.next(")"));
		return value;
	}

	u64 getU64(std::string name)
	{
		u64 value = 0;
		std::string s = get(name);
		std::istringstream ss(s);
		ss>>value;
		return value;
	}

	void setBool(std::string name, bool value)
	{
		if(value)
			set(name, "true");
		else
			set(name, "false");
	}

	void setS32(std::string name, s32 value)
	{
		set(name, itos(value));
	}

	void setFloat(std::string name, float value)
	{
		set(name, ftos(value));
	}

	void setV3F(std::string name, v3f value)
	{
		std::ostringstream os;
		os<<"("<<value.X<<","<<value.Y<<","<<value.Z<<")";
		set(name, os.str());
	}

	void setV2F(std::string name, v2f value)
	{
		std::ostringstream os;
		os<<"("<<value.X<<","<<value.Y<<")";
		set(name, os.str());
	}

	void setU64(std::string name, u64 value)
	{
		std::ostringstream os;
		os<<value;
		set(name, os.str());
	}

	void clear()
	{
		JMutexAutoLock lock(m_mutex);
		
		m_settings.clear();
		m_defaults.clear();
	}

	void updateValue(Settings &other, const std::string &name)
	{
		JMutexAutoLock lock(m_mutex);
		
		if(&other == this)
			return;

		try{
			std::string val = other.get(name);
			m_settings[name] = val;
		} catch(SettingNotFoundException &e){
		}

		return;
	}

	void update(Settings &other)
	{
		JMutexAutoLock lock(m_mutex);
		JMutexAutoLock lock2(other.m_mutex);
		
		if(&other == this)
			return;

		for(core::map<std::string, std::string>::Iterator
				i = other.m_settings.getIterator();
				i.atEnd() == false; i++)
		{
			m_settings[i.getNode()->getKey()] = i.getNode()->getValue();
		}
		
		for(core::map<std::string, std::string>::Iterator
				i = other.m_defaults.getIterator();
				i.atEnd() == false; i++)
		{
			m_defaults[i.getNode()->getKey()] = i.getNode()->getValue();
		}

		return;
	}

	Settings & operator+=(Settings &other)
	{
		JMutexAutoLock lock(m_mutex);
		JMutexAutoLock lock2(other.m_mutex);
		
		if(&other == this)
			return *this;

		for(core::map<std::string, std::string>::Iterator
				i = other.m_settings.getIterator();
				i.atEnd() == false; i++)
		{
			m_settings.insert(i.getNode()->getKey(),
					i.getNode()->getValue());
		}
		
		for(core::map<std::string, std::string>::Iterator
				i = other.m_defaults.getIterator();
				i.atEnd() == false; i++)
		{
			m_defaults.insert(i.getNode()->getKey(),
					i.getNode()->getValue());
		}

		return *this;

	}

	Settings & operator=(Settings &other)
	{
		JMutexAutoLock lock(m_mutex);
		JMutexAutoLock lock2(other.m_mutex);
		
		if(&other == this)
			return *this;

		clear();
		(*this) += other;
		
		return *this;
	}

private:
	core::map<std::string, std::string> m_settings;
	core::map<std::string, std::string> m_defaults;
	// All methods that access m_settings/m_defaults directly should lock this.
	JMutex m_mutex;
};

#endif

