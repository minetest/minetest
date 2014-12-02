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
#include "noise.h"
#include <cctype>
#include <algorithm>


Settings::~Settings()
{
	std::map<std::string, SettingsEntry>::const_iterator it;
	for (it = m_settings.begin(); it != m_settings.end(); ++it)
		delete it->second.group;
}


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


std::string Settings::sanitizeString(const std::string &value)
{
	std::string str = value;
	for (const char *s = "\t\n\v\f\r\b =\""; *s; s++)
		str.erase(std::remove(str.begin(), str.end(), *s), str.end());

	return str;
}


std::string Settings::getMultiline(std::istream &is)
{
	std::string value;
	std::string line;

	while (is.good()) {
		std::getline(is, line);
		if (line == "\"\"\"")
			break;
		value += line;
		value.push_back('\n');
	}

	size_t len = value.size();
	if (len)
		value.erase(len - 1);

	return value;
}


bool Settings::parseConfigLines(std::istream &is, const std::string &end)
{
	JMutexAutoLock lock(m_mutex);

	std::string line, name, value;

	while (is.good()) {
		std::getline(is, line);
		SettingsParseEvent event = parseConfigObject(line, end, name, value);

		switch (event) {
		case SPE_NONE:
		case SPE_INVALID:
		case SPE_COMMENT:
			break;
		case SPE_KVPAIR:
			m_settings[name] = SettingsEntry(value);
			break;
		case SPE_END:
			return true;
		case SPE_GROUP: {
			Settings *branch = new Settings;
			if (!branch->parseConfigLines(is, "}"))
				return false;

			m_settings[name] = SettingsEntry(branch);
			break;
		}
		case SPE_MULTILINE:
			m_settings[name] = SettingsEntry(getMultiline(is));
			break;
		}
	}

	return end.empty();
}


bool Settings::readConfigFile(const char *filename)
{
	std::ifstream is(filename);
	if (!is.good())
		return false;

	return parseConfigLines(is, "");
}


void Settings::writeLines(std::ostream &os, u32 tab_depth) const
{
	JMutexAutoLock lock(m_mutex);

	for (std::map<std::string, SettingsEntry>::const_iterator
			it = m_settings.begin();
			it != m_settings.end(); ++it) {
		bool is_multiline = it->second.value.find('\n') != std::string::npos;
		printValue(os, it->first, it->second, is_multiline, tab_depth);
	}
}


void Settings::printValue(std::ostream &os, const std::string &name,
	const SettingsEntry &entry, bool is_value_multiline, u32 tab_depth)
{
	for (u32 i = 0; i != tab_depth; i++)
		os << "\t";
	os << name << " = ";

	if (is_value_multiline)
		os << "\"\"\"\n" << entry.value << "\n\"\"\"\n";
	else
		os << entry.value << "\n";

	Settings *group = entry.group;
	if (group) {
		for (u32 i = 0; i != tab_depth; i++)
			os << "\t";

		os << name << " = {\n";
		group->writeLines(os, tab_depth + 1);

		for (u32 i = 0; i != tab_depth; i++)
			os << "\t";

		os << "}\n";
	}
}


bool Settings::updateConfigObject(std::istream &is, std::ostream &os,
	const std::string &end, u32 tab_depth)
{
	std::map<std::string, SettingsEntry>::const_iterator it;
	std::set<std::string> settings_in_config;
	bool was_modified = false;
	bool end_found = false;
	std::string line, name, value;

	// Add any settings that exist in the config file with the current value
	// in the object if existing
	while (is.good() && !end_found) {
		std::getline(is, line);
		SettingsParseEvent event = parseConfigObject(line, end, name, value);

		switch (event) {
		case SPE_END:
			end_found = true;
			break;
		case SPE_KVPAIR:
		case SPE_MULTILINE:
			it = m_settings.find(name);
			if (it != m_settings.end()) {
				if (event == SPE_MULTILINE)
					value = getMultiline(is);

				if (value != it->second.value) {
					value = it->second.value;
					was_modified = true;
				}
			}

			settings_in_config.insert(name);

			printValue(os, name, SettingsEntry(value),
				event == SPE_MULTILINE, tab_depth);

			break;
		case SPE_GROUP: {
			Settings *group = NULL;
			it = m_settings.find(name);
			if (it != m_settings.end())
				group = it->second.group;

			settings_in_config.insert(name);

			os << name << " = {\n";

			if (group) {
				was_modified |= group->updateConfigObject(is, os, "}", tab_depth + 1);
			} else {
				Settings dummy_settings;
				dummy_settings.updateConfigObject(is, os, "}", tab_depth + 1);
			}

			for (u32 i = 0; i != tab_depth; i++)
				os << "\t";
			os << "}\n";
			break;
		}
		default:
			os << line << (is.eof() ? "" : "\n");
			break;
		}
	}

	// Add any settings in the object that don't exist in the config file yet
	for (it = m_settings.begin(); it != m_settings.end(); ++it) {
		if (settings_in_config.find(it->first) != settings_in_config.end())
			continue;

		was_modified = true;

		bool is_multiline = it->second.value.find('\n') != std::string::npos;
		printValue(os, it->first, it->second, is_multiline, tab_depth);
	}

	return was_modified;
}


bool Settings::updateConfigFile(const char *filename)
{
	JMutexAutoLock lock(m_mutex);

	std::ifstream is(filename);
	std::ostringstream os(std::ios_base::binary);

	if (!updateConfigObject(is, os, ""))
		return true;

	if (!fs::safeWriteToFile(filename, os.str())) {
		errorstream << "Error writing configuration file: \""
			<< filename << "\"" << std::endl;
		return false;
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


const SettingsEntry &Settings::getEntry(const std::string &name) const
{
	JMutexAutoLock lock(m_mutex);

	std::map<std::string, SettingsEntry>::const_iterator n;
	if ((n = m_settings.find(name)) == m_settings.end()) {
		if ((n = m_defaults.find(name)) == m_defaults.end())
			throw SettingNotFoundException("Setting [" + name + "] not found.");
	}
	return n->second;
}


Settings *Settings::getGroup(const std::string &name) const
{
	Settings *group = getEntry(name).group;
	if (group == NULL)
		throw SettingNotFoundException("Setting [" + name + "] is not a group.");
	return group;
}


std::string Settings::get(const std::string &name) const
{
	return getEntry(name).value;
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


bool Settings::getNoiseParams(const std::string &name, NoiseParams &np) const
{
	return getNoiseParamsFromGroup(name, np) || getNoiseParamsFromValue(name, np);
}


bool Settings::getNoiseParamsFromValue(const std::string &name,
	NoiseParams &np) const
{
	std::string value;

	if (!getNoEx(name, value))
		return false;

	Strfnd f(value);

	np.offset   = stof(f.next(","));
	np.scale    = stof(f.next(","));
	f.next("(");
	np.spread.X = stof(f.next(","));
	np.spread.Y = stof(f.next(","));
	np.spread.Z = stof(f.next(")"));
	np.seed     = stoi(f.next(","));
	np.octaves  = stoi(f.next(","));
	np.persist  = stof(f.next(""));

	return true;
}


bool Settings::getNoiseParamsFromGroup(const std::string &name,
	NoiseParams &np) const
{
	Settings *group = NULL;

	if (!getGroupNoEx(name, group))
		return false;

	group->getFloatNoEx("offset",      np.offset);
	group->getFloatNoEx("scale",       np.scale);
	group->getV3FNoEx("spread",        np.spread);
	group->getS32NoEx("seed",          np.seed);
	group->getU16NoEx("octaves",       np.octaves);
	group->getFloatNoEx("persistence", np.persist);

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
	for (std::map<std::string, SettingsEntry>::const_iterator
			i = m_settings.begin();
			i != m_settings.end(); ++i) {
		names.push_back(i->first);
	}
	return names;
}



/***************************************
 * Getters that don't throw exceptions *
 ***************************************/

bool Settings::getEntryNoEx(const std::string &name, SettingsEntry &val) const
{
	try {
		val = getEntry(name);
		return true;
	} catch (SettingNotFoundException &e) {
		return false;
	}
}


bool Settings::getGroupNoEx(const std::string &name, Settings *&val) const
{
	try {
		val = getGroup(name);
		return true;
	} catch (SettingNotFoundException &e) {
		return false;
	}
}


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
bool Settings::getFlagStrNoEx(const std::string &name, u32 &val,
	FlagDesc *flagdesc) const
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


void Settings::set(const std::string &name, const std::string &value)
{
	{
	JMutexAutoLock lock(m_mutex);

	m_settings[name].value = value;
	}
	doCallbacks(name);
}


void Settings::setGroup(const std::string &name, Settings *group)
{
	JMutexAutoLock lock(m_mutex);

	delete m_settings[name].group;
	m_settings[name].group = group;
}


void Settings::setDefault(const std::string &name, const std::string &value)
{
	JMutexAutoLock lock(m_mutex);

	m_defaults[name].value = value;
}


void Settings::setGroupDefault(const std::string &name, Settings *group)
{
	JMutexAutoLock lock(m_mutex);

	delete m_defaults[name].group;
	m_defaults[name].group = group;
}


void Settings::setBool(const std::string &name, bool value)
{
	set(name, value ? "true" : "false");
}


void Settings::setS16(const std::string &name, s16 value)
{
	set(name, itos(value));
}


void Settings::setU16(const std::string &name, u16 value)
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


bool Settings::setStruct(const std::string &name, const std::string &format,
	void *value)
{
	std::string structstr;
	if (!serializeStructToString(&structstr, format, value))
		return false;

	set(name, structstr);
	return true;
}


void Settings::setNoiseParams(const std::string &name, const NoiseParams &np)
{
	Settings *group = new Settings;

	group->setFloat("offset",      np.offset);
	group->setFloat("scale",       np.scale);
	group->setV3F("spread",        np.spread);
	group->setS32("seed",          np.seed);
	group->setU16("octaves",       np.octaves);
	group->setFloat("persistence", np.persist);

	setGroup(name, group);
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


SettingsParseEvent Settings::parseConfigObject(const std::string &line,
	const std::string &end, std::string &name, std::string &value)
{
	std::string trimmed_line = trim(line);

	if (trimmed_line.empty())
		return SPE_NONE;
	if (trimmed_line[0] == '#')
		return SPE_COMMENT;
	if (trimmed_line == end)
		return SPE_END;

	size_t pos = trimmed_line.find('=');
	if (pos == std::string::npos)
		return SPE_INVALID;

	name  = trim(trimmed_line.substr(0, pos));
	value = trim(trimmed_line.substr(pos + 1));

	if (value == "{")
		return SPE_GROUP;
	if (value == "\"\"\"")
		return SPE_MULTILINE;

	return SPE_KVPAIR;
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


void Settings::registerChangedCallback(std::string name,
	setting_changed_callback cbf)
{
	m_callbacks[name].push_back(cbf);
}


void Settings::doCallbacks(const std::string name)
{
	std::vector<setting_changed_callback> tempvector;
	{
		JMutexAutoLock lock(m_mutex);
		if (m_callbacks.find(name) != m_callbacks.end())
		{
			tempvector = m_callbacks[name];
		}
	}

	for (std::vector<setting_changed_callback>::iterator iter = tempvector.begin();
			iter != tempvector.end(); iter ++)
	{
		(*iter)(name);
	}
}
