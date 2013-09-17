/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef SETTINGS_HEADER
#define SETTINGS_HEADER

#include "irrlichttypes_bloated.h"
#include "exceptions.h"
#include <string>
#include "jthread/jmutex.h"
#include "jthread/jmutexautolock.h"
#include "strfnd.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "debug.h"
#include "log.h"
#include "util/string.h"
#include "porting.h"
#include <list>
#include <map>
#include <set>
#include "filesys.h"

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

	void writeLines(std::ostream &os)
	{
		JMutexAutoLock lock(m_mutex);

		for(std::map<std::string, std::string>::iterator
				i = m_settings.begin();
				i != m_settings.end(); ++i)
		{
			std::string name = i->first;
			std::string value = i->second;
			os<<name<<" = "<<value<<"\n";
		}
	}
  
	// return all keys used 
	std::vector<std::string> getNames(){
		std::vector<std::string> names;
		for(std::map<std::string, std::string>::iterator
				i = m_settings.begin();
				i != m_settings.end(); ++i)
		{
			names.push_back(i->first);
		}
		return names;  
	}

	// remove a setting
	bool remove(const std::string& name)
	{
		return m_settings.erase(name);
	}


	bool parseConfigLine(const std::string &line)
	{
		JMutexAutoLock lock(m_mutex);

		std::string trimmedline = trim(line);

		// Ignore empty lines and comments
		if(trimmedline.size() == 0 || trimmedline[0] == '#')
			return true;

		//infostream<<"trimmedline=\""<<trimmedline<<"\""<<std::endl;

		Strfnd sf(trim(line));

		std::string name = sf.next("=");
		name = trim(name);

		if(name == "")
			return true;

		std::string value = sf.next("\n");
		value = trim(value);

		/*infostream<<"Config name=\""<<name<<"\" value=\""
				<<value<<"\""<<std::endl;*/

		m_settings[name] = value;

		return true;
	}

	void parseConfigLines(std::istream &is, const std::string &endstring)
	{
		for(;;){
			if(is.eof())
				break;
			std::string line;
			std::getline(is, line);
			std::string trimmedline = trim(line);
			if(endstring != ""){
				if(trimmedline == endstring)
					break;
			}
			parseConfigLine(line);
		}
	}

	// Returns false on EOF
	bool parseConfigObject(std::istream &is)
	{
		if(is.eof())
			return false;

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
		Read configuration file

		Returns true on success
	*/
	bool readConfigFile(const char *filename)
	{
		std::ifstream is(filename);
		if(is.good() == false)
			return false;

		/*infostream<<"Parsing configuration file: \""
				<<filename<<"\""<<std::endl;*/

		while(parseConfigObject(is));

		return true;
	}

	/*
		Reads a configuration object from stream (usually a single line)
		and adds it to dst.

		Preserves comments and empty lines.

		Settings that were added to dst are also added to updated.
		key of updated is setting name, value of updated is dummy.

		Returns false on EOF
	*/
	bool getUpdatedConfigObject(std::istream &is,
			std::list<std::string> &dst,
			std::set<std::string> &updated,
			bool &value_changed)
	{
		JMutexAutoLock lock(m_mutex);

		if(is.eof())
			return false;

		// NOTE: This function will be expanded to allow multi-line settings
		std::string line;
		std::getline(is, line);

		std::string trimmedline = trim(line);

		std::string line_end = "";
		if(is.eof() == false)
			line_end = "\n";

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

		if(m_settings.find(name) != m_settings.end())
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

			updated.insert(name);
		}
		else //file contains a setting which is not in m_settings
			value_changed=true;
			
		return true;
	}

	/*
		Updates configuration file

		Returns true on success
	*/
	bool updateConfigFile(const char *filename)
	{
		infostream<<"Updating configuration file: \""
				<<filename<<"\""<<std::endl;

		std::list<std::string> objects;
		std::set<std::string> updated;
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
		if(!something_actually_changed){
			for(std::map<std::string, std::string>::iterator
					i = m_settings.begin();
					i != m_settings.end(); ++i)
			{
				if(updated.find(i->first) != updated.end())
					continue;
				something_actually_changed = true;
				break;
			}
		}

		// If nothing was actually changed, skip writing the file
		if(!something_actually_changed){
			infostream<<"Skipping writing of "<<filename
					<<" because content wouldn't be modified"<<std::endl;
			return true;
		}

		// Write stuff back
		{
			std::ostringstream ss(std::ios_base::binary);

			/*
				Write updated stuff
			*/
			for(std::list<std::string>::iterator
					i = objects.begin();
					i != objects.end(); ++i)
			{
				ss<<(*i);
			}

			/*
				Write stuff that was not already in the file
			*/
			for(std::map<std::string, std::string>::iterator
					i = m_settings.begin();
					i != m_settings.end(); ++i)
			{
				if(updated.find(i->first) != updated.end())
					continue;
				std::string name = i->first;
				std::string value = i->second;
				infostream<<"Adding \""<<name<<"\" = \""<<value<<"\""
						<<std::endl;
				ss<<name<<" = "<<value<<"\n";
			}

			if(!fs::safeWriteToFile(filename, ss.str()))
			{
				errorstream<<"Error writing configuration file: \""
						<<filename<<"\""<<std::endl;
				return false;
			}
		}

		return true;
	}

	/*
		NOTE: Types of allowed_options are ignored

		returns true on success
	*/
	bool parseCommandLine(int argc, char *argv[],
			std::map<std::string, ValueSpec> &allowed_options)
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

			std::map<std::string, ValueSpec>::iterator n;
			n = allowed_options.find(name);
			if(n == allowed_options.end())
			{
				errorstream<<"Unknown command-line parameter \""
						<<argname<<"\""<<std::endl;
				return false;
			}

			ValueType type = n->second.type;

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

		return (m_settings.find(name) != m_settings.end() || m_defaults.find(name) != m_defaults.end());
	}

	std::string get(std::string name)
	{
		JMutexAutoLock lock(m_mutex);

		std::map<std::string, std::string>::iterator n;
		n = m_settings.find(name);
		if(n == m_settings.end())
		{
			n = m_defaults.find(name);
			if(n == m_defaults.end())
			{
				throw SettingNotFoundException(("Setting [" + name + "] not found ").c_str());
			}
		}

		return n->second;
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

	u32 getFlagStr(std::string name, FlagDesc *flagdesc)
	{
		std::string val = get(name);
		return (isdigit(val[0])) ? stoi(val) : readFlagString(val, flagdesc);
	}

	bool getStruct(std::string name, std::string format, void *out, size_t olen)
	{
		size_t len = olen;
		std::vector<std::string *> strs_alloced;
		std::string *str;
		std::string valstr = get(name);
		char *s = &valstr[0];
		char *buf = new char[len];
		char *bufpos = buf;
		char *f, *snext;
		size_t pos;

		char *fmtpos, *fmt = &format[0];
		while ((f = strtok_r(fmt, ",", &fmtpos)) && s) {
			fmt = NULL;

			bool is_unsigned = false;
			int width = 0;
			char valtype = *f;

			width = (int)strtol(f + 1, &f, 10);
			if (width && valtype == 's')
				valtype = 'i';

			switch (valtype) {
				case 'u':
					is_unsigned = true;
					/* FALLTHROUGH */
				case 'i':
					if (width == 16) {
						bufpos += PADDING(bufpos, u16);
						if ((bufpos - buf) + sizeof(u16) <= len) {
							if (is_unsigned)
								*(u16 *)bufpos = (u16)strtoul(s, &s, 10);
							else
								*(s16 *)bufpos = (s16)strtol(s, &s, 10);
						}
						bufpos += sizeof(u16);
					} else if (width == 32) {
						bufpos += PADDING(bufpos, u32);
						if ((bufpos - buf) + sizeof(u32) <= len) {
							if (is_unsigned)
								*(u32 *)bufpos = (u32)strtoul(s, &s, 10);
							else
								*(s32 *)bufpos = (s32)strtol(s, &s, 10);
						}
						bufpos += sizeof(u32);
					} else if (width == 64) {
						bufpos += PADDING(bufpos, u64);
						if ((bufpos - buf) + sizeof(u64) <= len) {
							if (is_unsigned)
								*(u64 *)bufpos = (u64)strtoull(s, &s, 10);
							else
								*(s64 *)bufpos = (s64)strtoll(s, &s, 10);
						}
						bufpos += sizeof(u64);
					}
					s = strchr(s, ',');
					break;
				case 'b':
					snext = strchr(s, ',');
					if (snext)
						*snext++ = 0;

					bufpos += PADDING(bufpos, bool);
					if ((bufpos - buf) + sizeof(bool) <= len)
						*(bool *)bufpos = is_yes(std::string(s));
					bufpos += sizeof(bool);

					s = snext;
					break;
				case 'f':
					bufpos += PADDING(bufpos, float);
					if ((bufpos - buf) + sizeof(float) <= len)
						*(float *)bufpos = strtof(s, &s);
					bufpos += sizeof(float);

					s = strchr(s, ',');
					break;
				case 's':
					while (*s == ' ' || *s == '\t')
						s++;
					if (*s++ != '"') //error, expected string
						goto fail;
					snext = s;

					while (snext[0] && !(snext[-1] != '\\' && snext[0] == '"'))
						snext++;
					*snext++ = 0;

					bufpos += PADDING(bufpos, std::string *);

					str = new std::string(s);
					pos = 0;
					while ((pos = str->find("\\\"", pos)) != std::string::npos)
						str->erase(pos, 1);

					if ((bufpos - buf) + sizeof(std::string *) <= len)
						*(std::string **)bufpos = str;
					bufpos += sizeof(std::string *);
					strs_alloced.push_back(str);

					s = *snext ? snext + 1 : NULL;
					break;
				case 'v':
					while (*s == ' ' || *s == '\t')
						s++;
					if (*s++ != '(') //error, expected vector
						goto fail;

					if (width == 2) {
						bufpos += PADDING(bufpos, v2f);

						if ((bufpos - buf) + sizeof(v2f) <= len) {
						v2f *v = (v2f *)bufpos;
							v->X = strtof(s, &s);
							s++;
							v->Y = strtof(s, &s);
						}

						bufpos += sizeof(v2f);
					} else if (width == 3) {
						bufpos += PADDING(bufpos, v3f);
						if ((bufpos - buf) + sizeof(v3f) <= len) {
							v3f *v = (v3f *)bufpos;
							v->X = strtof(s, &s);
							s++;
							v->Y = strtof(s, &s);
							s++;
							v->Z = strtof(s, &s);
						}

						bufpos += sizeof(v3f);
					}
					s = strchr(s, ',');
					break;
				default: //error, invalid format specifier
					goto fail;
			}

			if (s && *s == ',')
				s++;

			if ((size_t)(bufpos - buf) > len) //error, buffer too small
				goto fail;
		}

		if (f && *f) { //error, mismatched number of fields and values
fail:
			for (size_t i = 0; i != strs_alloced.size(); i++)
				delete strs_alloced[i];
			delete[] buf;
			return false;
		}

		memcpy(out, buf, olen);
		delete[] buf;
		return true;
	}

	bool setStruct(std::string name, std::string format, void *value)
	{
		char sbuf[2048];
		int sbuflen = sizeof(sbuf) - 1;
		sbuf[sbuflen] = 0;
		std::string str;
		int pos = 0;
		size_t fpos;
		char *f;

		char *bufpos = (char *)value;
		char *fmtpos, *fmt = &format[0];
		while ((f = strtok_r(fmt, ",", &fmtpos))) {
			fmt = NULL;
			bool is_unsigned = false;
			int width = 0, nprinted = 0;
			char valtype = *f;

			width = (int)strtol(f + 1, &f, 10);
			if (width && valtype == 's')
				valtype = 'i';

			switch (valtype) {
				case 'u':
					is_unsigned = true;
					/* FALLTHROUGH */
				case 'i':
					if (width == 16) {
						bufpos += PADDING(bufpos, u16);
						nprinted = snprintf(sbuf + pos, sbuflen,
									is_unsigned ? "%u, " : "%d, ",
									*((u16 *)bufpos));
						bufpos += sizeof(u16);
					} else if (width == 32) {
						bufpos += PADDING(bufpos, u32);
						nprinted = snprintf(sbuf + pos, sbuflen,
									is_unsigned ? "%u, " : "%d, ",
									*((u32 *)bufpos));
						bufpos += sizeof(u32);
					} else if (width == 64) {
						bufpos += PADDING(bufpos, u64);
						nprinted = snprintf(sbuf + pos, sbuflen,
									is_unsigned ? "%llu, " : "%lli, ",
									(unsigned long long)*((u64 *)bufpos));
						bufpos += sizeof(u64);
					}
					break;
				case 'b':
					bufpos += PADDING(bufpos, bool);
					nprinted = snprintf(sbuf + pos, sbuflen, "%s, ",
										*((bool *)bufpos) ? "true" : "false");
					bufpos += sizeof(bool);
					break;
				case 'f':
					bufpos += PADDING(bufpos, float);
					nprinted = snprintf(sbuf + pos, sbuflen, "%f, ",
										*((float *)bufpos));
					bufpos += sizeof(float);
					break;
				case 's':
					bufpos += PADDING(bufpos, std::string *);
					str = **((std::string **)bufpos);

					fpos = 0;
					while ((fpos = str.find('"', fpos)) != std::string::npos) {
						str.insert(fpos, 1, '\\');
						fpos += 2;
					}

					nprinted = snprintf(sbuf + pos, sbuflen, "\"%s\", ",
										(*((std::string **)bufpos))->c_str());
					bufpos += sizeof(std::string *);
					break;
				case 'v':
					if (width == 2) {
						bufpos += PADDING(bufpos, v2f);
						v2f *v = (v2f *)bufpos;
						nprinted = snprintf(sbuf + pos, sbuflen,
											"(%f, %f), ", v->X, v->Y);
						bufpos += sizeof(v2f);
					} else {
						bufpos += PADDING(bufpos, v3f);
						v3f *v = (v3f *)bufpos;
						nprinted = snprintf(sbuf + pos, sbuflen,
											"(%f, %f, %f), ", v->X, v->Y, v->Z);
						bufpos += sizeof(v3f);
					}
					break;
				default:
					return false;
			}
			if (nprinted < 0) //error, buffer too small
				return false;
			pos     += nprinted;
			sbuflen -= nprinted;
		}

		if (pos >= 2)
			sbuf[pos - 2] = 0;

		set(name, std::string(sbuf));
		return true;
	}
	
	void setFlagStr(std::string name, u32 flags, FlagDesc *flagdesc)
	{
		set(name, writeFlagString(flags, flagdesc));
	}

	void setBool(std::string name, bool value)
	{
		if(value)
			set(name, "true");
		else
			set(name, "false");
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

	void setS16(std::string name, s16 value)
	{
		set(name, itos(value));
	}

	void setS32(std::string name, s32 value)
	{
		set(name, itos(value));
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

		m_settings.insert(other.m_settings.begin(), other.m_settings.end());
		m_defaults.insert(other.m_defaults.begin(), other.m_defaults.end());

		return;
	}

	Settings & operator+=(Settings &other)
	{
		JMutexAutoLock lock(m_mutex);
		JMutexAutoLock lock2(other.m_mutex);

		if(&other == this)
			return *this;

		update(other);

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
	std::map<std::string, std::string> m_settings;
	std::map<std::string, std::string> m_defaults;
	// All methods that access m_settings/m_defaults directly should lock this.
	JMutex m_mutex;
};

#endif

