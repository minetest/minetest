#include "itemstackmetadata.h"
#include "util/serialize.h"
#include "util/strfnd.h"
#include <json/json.h>

#define SERIALIZATION_VERSION_IDENTIFIER '\x02'

#define TOOLCAP_KEY "tool_capabilities"

void ItemStackMetadata::clear()
{
	Metadata::clear();
	updateToolCapabilities();
}

bool ItemStackMetadata::setString(const std::string &name, const std::string &var)
{
	bool result = Metadata::setString(name, var);
	if (name == TOOLCAP_KEY)
		updateToolCapabilities();
	return result;
}

void ItemStackMetadata::serialize(std::ostream &os) const
{
	Json::Value root(Json::objectValue);
	for (const auto &stringvar : m_stringvars) {
		if (!stringvar.first.empty() || !stringvar.second.empty())
			root[stringvar.first] = stringvar.second;
	}
	os << serializeJsonStringIfNeeded((char)SERIALIZATION_VERSION_IDENTIFIER +
			Json::FastWriter().write(root));
}

#define DESERIALIZE_START '\x01'
#define DESERIALIZE_KV_DELIM '\x02'
#define DESERIALIZE_PAIR_DELIM '\x03'
#define DESERIALIZE_START_STR "\x01"
#define DESERIALIZE_KV_DELIM_STR "\x02"
#define DESERIALIZE_PAIR_DELIM_STR "\x03"

void ItemStackMetadata::deSerialize(std::istream &is)
{
	std::string in = deSerializeJsonStringIfNeeded(is);

	m_stringvars.clear();

	if (in.empty())
		return;

	if (in[0] == SERIALIZATION_VERSION_IDENTIFIER) {
		in.erase(0, 1);

		Json::Reader reader;
		Json::Value attr_root;
		if (reader.parse(in, attr_root) && attr_root.type() == Json::objectValue) {
			const Json::Value::Members attr_list = attr_root.getMemberNames();
			for (Json::Value::Members::const_iterator it = attr_list.begin();
					it != attr_list.end(); ++it) {
				m_stringvars[*it] = attr_root[*it].asString();
			}
		} else {
			errorstream << "ItemStackMetadata::deSerialize():"
				<< " expected JSON data, but failed to parse. Ignoring." << std::endl;
		}
	} else if (in[0] == DESERIALIZE_START) {
		// BACKWARDS COMPATIBILITY
		Strfnd fnd(in);
		fnd.to(1);
		while (!fnd.at_end()) {
			std::string name = fnd.next(DESERIALIZE_KV_DELIM_STR);
			std::string var  = fnd.next(DESERIALIZE_PAIR_DELIM_STR);
			m_stringvars[name] = var;
		}
	} else {
		// BACKWARDS COMPATIBILITY
		m_stringvars[""] = in;
	}
	updateToolCapabilities();
}

void ItemStackMetadata::updateToolCapabilities()
{
	if (contains(TOOLCAP_KEY)) {
		toolcaps_overridden = true;
		toolcaps_override = ToolCapabilities();
		std::istringstream is(getString(TOOLCAP_KEY));
		toolcaps_override.deserializeJson(is);
	} else {
		toolcaps_overridden = false;
	}
}

void ItemStackMetadata::setToolCapabilities(const ToolCapabilities &caps)
{
	std::ostringstream os;
	caps.serializeJson(os);
	setString(TOOLCAP_KEY, os.str());
}

void ItemStackMetadata::clearToolCapabilities()
{
	setString(TOOLCAP_KEY, "");
}
