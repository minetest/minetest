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

#include "settings.h"
#include "irrlichttypes_bloated.h"
#include "exceptions.h"
#include "jthread/jmutexautolock.h"
#include "strfnd.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include "debug.h"
#include "log.h"
#include "util/serialize.h"
#include "filesys.h"
#include <cctype>


Settings & Settings::operator += (const Settings &other)
{
	update(other);

	return *this;
}


Settings & Settings::operator = (const Settings &other)
{
	if (&other == this)
		return *this;

	JMutexAutoLock lock(m_mutex);
	JMutexAutoLock lock2(other.m_mutex);

	clearNoLock();
	updateNoLock(other);

	return *this;
}


bool Settings::parseConfigLines(std::istream &is,
		const std::string &end)
{
	JMutexAutoLock lock(m_mutex);

	std::string name, value;
	bool end_found = false;

	while (is.good() && !end_found) {
		if (parseConfigObject(is, name, value, end, end_found)) {
			m_settings[name] = value;
		}
	}
	if (!end.empty() && !end_found) {
		return false;
	}
	return true;
}


bool Settings::readConfigFile(const char *filename)
{
	std::ifstream is(filename);
	if (!is.good())
		return false;

	JMutexAutoLock lock(m_mutex);

	std::string name, value;

	while (is.good()) {
		if (parseConfigObject(is, name, value)) {
			m_settings[name] = value;
		}
	}

	return true;
}


void Settings::writeLines(std::ostream &os) const
{
	JMutexAutoLock lock(m_mutex);

	for (std::map<std::string, std::string>::const_iterator
			i = m_settings.begin();
			i != m_settings.end(); ++i) {
		os << i->first << " = " << i->second << '\n';
	}
}


bool Settings::updateConfigFile(const char *filename)
{
	std::list<std::string> objects;
	std::set<std::string> updated;
	bool changed = false;

	JMutexAutoLock lock(m_mutex);

	// Read the file and check for differences
	{
		std::ifstream is(filename);
		while (is.good()) {
			getUpdatedConfigObject(is, objects,
					updated, changed);
		}
	}

	// If something not yet determined to have been changed, check if
	// any new stuff was added
	if (!changed) {
		for (std::map<std::string, std::string>::const_iterator
				i = m_settings.begin();
				i != m_settings.end(); ++i) {
			if (updated.find(i->first) == updated.end()) {
				changed = true;
				break;
			}
		}
	}

	// If nothing was actually changed, skip writing the file
	if (!changed) {
		return true;
	}

	// Write stuff back
	{
		std::ostringstream ss(std::ios_base::binary);

		// Write changes settings
		for (std::list<std::string>::const_iterator
				i = objects.begin();
				i != objects.end(); ++i) {
			ss << (*i);
		}

		// Write new settings
		for (std::map<std::string, std::string>::const_iterator
				i = m_settings.begin();
				i != m_settings.end(); ++i) {
			if (updated.find(i->first) != updated.end())
				continue;
			ss << i->first << " = " << i->second << '\n';
		}

		if (!fs::safeWriteToFile(filename, ss.str())) {
			errorstream << "Error writing configuration file: \""
					<< filename << "\"" << std::endl;
			return false;
		}
	}

	return true;
}


bool Settings::parseCommandLine(int argc, char *argv[],
		std::map<std::string, ValueSpec> &allowed_options)
{
	int nonopt_index = 0;
	for (int i = 1; i < argc; i++) {
		std::string arg_name = argv[i];
		if (arg_name.substr(0, 2) != "--") {
			// If option doesn't start with -, read it in as nonoptX
			if (arg_name[0] != '-'){
				std::string name = "nonopt";
				name += itos(nonopt_index);
				set(name, arg_name);
				nonopt_index++;
				continue;
			}
			errorstream << "Invalid command-line parameter \""
					<< arg_name << "\": --<option> expected." << std::endl;
			return false;
		}

		std::string name = arg_name.substr(2);

		std::map<std::string, ValueSpec>::iterator n;
		n = allowed_options.find(name);
		if (n == allowed_options.end()) {
			errorstream << "Unknown command-line parameter \""
					<< arg_name << "\"" << std::endl;
			return false;
		}

		ValueType type = n->second.type;

		std::string value = "";

		if (type == VALUETYPE_FLAG) {
			value = "true";
		} else {
			if ((i + 1) >= argc) {
				errorstream << "Invalid command-line parameter \""
						<< name << "\": missing value" << std::endl;
				return false;
			}
			value = argv[++i];
		}

		set(name, value);
	}

	return true;
}



/***********
 * Getters *
 ***********/


std::string Settings::get(const std::string &name) const
{
	JMutexAutoLock lock(m_mutex);

	std::map<std::string, std::string>::const_iterator n;
	if ((n = m_settings.find(name)) == m_settings.end()) {
		if ((n = m_defaults.find(name)) == m_defaults.end()) {
			throw SettingNotFoundException("Setting [" + name + "] not found.");
		}
	}
	return n->second;
}


bool Settings::getBool(const std::string &name) const
{
	return is_yes(get(name));
}


u16 Settings::getU16(const std::string &name) const
{
	return stoi(get(name), 0, 65535);
}


s16 Settings::getS16(const std::string &name) const
{
	return stoi(get(name), -32768, 32767);
}


s32 Settings::getS32(const std::string &name) const
{
	return stoi(get(name));
}


float Settings::getFloat(const std::string &name) const
{
	return stof(get(name));
}


u64 Settings::getU64(const std::string &name) const
{
	u64 value = 0;
	std::string s = get(name);
	std::istringstream ss(s);
	ss >> value;
	return value;
}


v2f Settings::getV2F(const std::string &name) const
{
	v2f value;
	Strfnd f(get(name));
	f.next("(");
	value.X = stof(f.next(","));
	value.Y = stof(f.next(")"));
	return value;
}


v3f Settings::getV3F(const std::string &name) const
{
	v3f value;
	Strfnd f(get(name));
	f.next("(");
	value.X = stof(f.next(","));
	value.Y = stof(f.next(","));
	value.Z = stof(f.next(")"));
	return value;
}


u32 Settings::getFlagStr(const std::string &name, const FlagDesc *flagdesc,
		u32 *flagmask) const
{
	std::string val = get(name);
	return std::isdigit(val[0])
		? stoi(val)
		: readFlagString(val, flagdesc, flagmask);
}


// N.B. if getStruct() is used to read a non-POD aggregate type,
// the behavior is undefined.
bool Settings::getStruct(const std::string &name, const std::string &format,
		void *out, size_t olen) const
{
	std::string valstr;

	try {
		valstr = get(name);
	} catch (SettingNotFoundException &e) {
		return false;
	}

	if (!deSerializeStringToStruct(valstr, format, out, olen))
		return false;

	return true;
}


bool Settings::exists(const std::string &name) const
{
	JMutexAutoLock lock(m_mutex);

	return (m_settings.find(name) != m_settings.end() ||
		m_defaults.find(name) != m_defaults.end());
}


std::vector<std::string> Settings::getNames() const
{
	std::vector<std::string> names;
	for (std::map<std::string, std::string>::const_iterator
			i = m_settings.begin();
			i != m_settings.end(); ++i) {
		names.push_back(i->first);
	}
	return names;
}



/***************************************
 * Getters that don't throw exceptions *
 ***************************************/


bool Settings::getNoEx(const std::string &name, std::string &val) const
{
	try {
		val = get(name);
		return true;
	} catch (SettingNotFoundException &e) {
		return false;
	}
}


bool Settings::getFlag(const std::string &name) const
{
	try {
		return getBool(name);
	} catch(SettingNotFoundException &e) {
		return false;
	}
}


bool Settings::getFloatNoEx(const std::string &name, float &val) const
{
	try {
		val = getFloat(name);
		return true;
	} catch (SettingNotFoundException &e) {
		return false;
	}
}


bool Settings::getU16NoEx(const std::string &name, u16 &val) const
{
	try {
		val = getU16(name);
		return true;
	} catch (SettingNotFoundException &e) {
		return false;
	}
}


bool Settings::getS16NoEx(const std::string &name, s16 &val) const
{
	try {
		val = getS16(name);
		return true;
	} catch (SettingNotFoundException &e) {
		return false;
	}
}


bool Settings::getS32NoEx(const std::string &name, s32 &val) const
{
	try {
		val = getS32(name);
		return true;
	} catch (SettingNotFoundException &e) {
		return false;
	}
}


bool Settings::getU64NoEx(const std::string &name, u64 &val) const
{
	try {
		val = getU64(name);
		return true;
	} catch (SettingNotFoundException &e) {
		return false;
	}
}


bool Settings::getV2FNoEx(const std::string &name, v2f &val) const
{
	try {
		val = getV2F(name);
		return true;
	} catch (SettingNotFoundException &e) {
		return false;
	}
}


bool Settings::getV3FNoEx(const std::string &name, v3f &val) const
{
	try {
		val = getV3F(name);
		return true;
	} catch (SettingNotFoundException &e) {
		return false;
	}
}


// N.B. getFlagStrNoEx() does not set val, but merely modifies it.  Thus,
// val must be initialized before using getFlagStrNoEx().  The intention of
// this is to simplify modifying a flags field from a default value.
bool Settings::getFlagStrNoEx(const std::string &name, u32 &val, FlagDesc *flagdesc) const
{
	try {
		u32 flags, flagmask;

		flags = getFlagStr(name, flagdesc, &flagmask);

		val &= ~flagmask;
		val |=  flags;

		return true;
	} catch (SettingNotFoundException &e) {
		return false;
	}
}


	
/***********
 * Setters *
 ***********/


void Settings::set(const std::string &name, std::string value)
{
	JMutexAutoLock lock(m_mutex);

	m_settings[name] = value;
}


void Settings::set(const std::string &name, const char *value)
{
	JMutexAutoLock lock(m_mutex);

	m_settings[name] = value;
}


void Settings::setDefault(const std::string &name, std::string value)
{
	JMutexAutoLock lock(m_mutex);

	m_defaults[name] = value;
}


void Settings::setBool(const std::string &name, bool value)
{
	set(name, value ? "true" : "false");
}


void Settings::setS16(const std::string &name, s16 value)
{
	set(name, itos(value));
}


void Settings::setS32(const std::string &name, s32 value)
{
	set(name, itos(value));
}


void Settings::setU64(const std::string &name, u64 value)
{
	std::ostringstream os;
	os << value;
	set(name, os.str());
}


void Settings::setFloat(const std::string &name, float value)
{
	set(name, ftos(value));
}


void Settings::setV2F(const std::string &name, v2f value)
{
	std::ostringstream os;
	os << "(" << value.X << "," << value.Y << ")";
	set(name, os.str());
}


void Settings::setV3F(const std::string &name, v3f value)
{
	std::ostringstream os;
	os << "(" << value.X << "," << value.Y << "," << value.Z << ")";
	set(name, os.str());
}


void Settings::setFlagStr(const std::string &name, u32 flags,
	const FlagDesc *flagdesc, u32 flagmask)
{
	set(name, writeFlagString(flags, flagdesc, flagmask));
}


bool Settings::setStruct(const std::string &name, const std::string &format, void *value)
{
	std::string structstr;
	if (!serializeStructToString(&structstr, format, value))
		return false;

	set(name, structstr);
	return true;
}


bool Settings::remove(const std::string &name)
{
	JMutexAutoLock lock(m_mutex);
	return m_settings.erase(name);
}


void Settings::clear()
{
	JMutexAutoLock lock(m_mutex);
	clearNoLock();
}


void Settings::updateValue(const Settings &other, const std::string &name)
{
	if (&other == this)
		return;

	JMutexAutoLock lock(m_mutex);

	try {
		std::string val = other.get(name);
		m_settings[name] = val;
	} catch (SettingNotFoundException &e) {
	}
}


void Settings::update(const Settings &other)
{
	if (&other == this)
		return;

	JMutexAutoLock lock(m_mutex);
	JMutexAutoLock lock2(other.m_mutex);

	updateNoLock(other);
}


inline bool Settings::parseConfigObject(std::istream &is,
		std::string &name, std::string &value)
{
	bool end_found = false;
	return parseConfigObject(is, name, value, "", end_found);
}


// NOTE: This function might be expanded to allow multi-line settings.
bool Settings::parseConfigObject(std::istream &is,
		std::string &name, std::string &value,
		const std::string &end, bool &end_found)
{
	std::string line;
	std::getline(is, line);
	std::string trimmed_line = trim(line);

	// Ignore empty lines and comments
	if (trimmed_line.empty() || trimmed_line[0] == '#') {
		value = trimmed_line;
		return false;
	}
	if (trimmed_line == end) {
		end_found = true;
		return false;
	}

	Strfnd sf(trimmed_line);

	name = trim(sf.next("="));
	if (name.empty()) {
		value = trimmed_line;
		return false;
	}

	value = trim(sf.next("\n"));

	return true;
}


void Settings::getUpdatedConfigObject(std::istream &is,
		std::list<std::string> &dst,
		std::set<std::string> &updated,
		bool &changed)
{
	std::string name, value;
	if (!parseConfigObject(is, name, value)) {
		dst.push_back(value + '\n');
		return;
	}

	if (m_settings.find(name) != m_settings.end()) {
		std::string new_value = m_settings[name];

		if (new_value != value) {
			changed = true;
		}

		dst.push_back(name + " = " + new_value + '\n');
		updated.insert(name);
	} else { // File contains a setting which is not in m_settings
		changed = true;
	}
}


void Settings::updateNoLock(const Settings &other)
{
	m_settings.insert(other.m_settings.begin(), other.m_settings.end());
	m_defaults.insert(other.m_defaults.begin(), other.m_defaults.end());
}


void Settings::clearNoLock()
{
	m_settings.clear();
	m_defaults.clear();
}

