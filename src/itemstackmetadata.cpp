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
	for (const auto &entry : m_stringvars) {
		if (!entry.first.empty() || !entry.second.empty())
			root[entry.first] = entry.second;
	}

	Json::StreamWriterBuilder writerBuilder;
	os << SERIALIZATION_VERSION_IDENTIFIER
	   << Json::writeString(writerBuilder, root);
}

#define DESERIALIZE_START '\x01'
#define DESERIALIZE_KV_DELIM '\x02'
#define DESERIALIZE_PAIR_DELIM '\x03'
#define DESERIALIZE_START_STR "\x01"
#define DESERIALIZE_KV_DELIM_STR "\x02"
#define DESERIALIZE_PAIR_DELIM_STR "\x03"

void ItemStackMetadata::deSerialize(std::istream &is)
{
	m_stringvars.clear();

	// Check for EOF
	if (!is.good())
		return;

	// Decode Json metadata (ItemMeta 3, 2017-12-22)
	if (is.peek() == SERIALIZATION_VERSION_IDENTIFIER) {
		is.ignore(1);

		Json::Value attr_root;
		is >> attr_root;
		if (attr_root.type() == Json::objectValue) {
			const Json::Value::Members attr_list = attr_root.getMemberNames();
			for (const auto& entry : attr_list) {
				m_stringvars[entry] = attr_root[entry].asString();
			}
		} else {
			errorstream << "ItemStackMetadata::deSerialize():"
				<< " expected JSON data, but failed to parse. Ignoring." << std::endl;
		}

	} else {
		std::string in = deSerializeJsonStringIfNeeded(is);

		// BACKWARDS COMPATIBILITY (ItemMeta 2, 2017-01-31)
		if (in[0] == DESERIALIZE_START) {
			Strfnd fnd(in);
			fnd.to(1);
			while (!fnd.at_end()) {
				std::string name = fnd.next(DESERIALIZE_KV_DELIM_STR);
				std::string var  = fnd.next(DESERIALIZE_PAIR_DELIM_STR);
				m_stringvars[name] = var;
			}

		// BACKWARDS COMPATIBILITY (ItemMeta 1)
		} else {
			m_stringvars[""] = in;
		}
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
