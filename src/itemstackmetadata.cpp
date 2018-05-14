#include "itemstackmetadata.h"
#include "util/serialize.h"
#include "util/strfnd.h"

#define DESERIALIZE_START '\x01'
#define DESERIALIZE_KV_DELIM '\x02'
#define DESERIALIZE_PAIR_DELIM '\x03'
#define DESERIALIZE_START_STR "\x01"
#define DESERIALIZE_KV_DELIM_STR "\x02"
#define DESERIALIZE_PAIR_DELIM_STR "\x03"

void ItemStackMetadata::serialize(std::ostream &os) const
{
	std::ostringstream os2;
	os2 << DESERIALIZE_START;
	for (StringMap::const_iterator it = m_stringvars.begin(); it != m_stringvars.end();
		++it) {
		if (!(*it).first.empty() || !(*it).second.empty())
			os2 << (*it).first << DESERIALIZE_KV_DELIM
				<< (*it).second << DESERIALIZE_PAIR_DELIM;
	}
	os << serializeJsonStringIfNeeded(os2.str());
}

void ItemStackMetadata::deSerialize(std::istream &is)
{
	std::string in = deSerializeJsonStringIfNeeded(is);

	m_stringvars.clear();

	if (!in.empty()) {
		if (in[0] == DESERIALIZE_START) {
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
	}
}
