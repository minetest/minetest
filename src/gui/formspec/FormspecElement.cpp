#include "FormspecElement.h"
#include "FormspecParseException.h"
#include <sstream>

void FormspecElement::checkLength(bool relaxed, size_t expectedSize) const {
	size_t actualSize = size();
	if (actualSize == expectedSize || (relaxed && actualSize > expectedSize))
		return;

	std::ostringstream os;
	os << "Incorrect number of parts (" << actualSize << " != " << expectedSize << ")";
	throw FormspecParseException(true, os.str());
}

v2f32 FormspecElement::getV2f(size_t idx) const {
	auto v = split(parts[idx], ',');
	if (v.size() != 2) {
		std::ostringstream os;
		os << "Invalid pos at argument " << (idx + 1);
		throw FormspecParseException(true, os.str());
	}

	return v2f32(stof(v[0]), stof(v[1]));
}

v2s32 FormspecElement::getV2s(size_t idx) const {
	auto v = split(parts[idx], ',');
	if (v.size() != 2) {
		std::ostringstream os;
		os << "Invalid pos at argument " << (idx + 1);
		throw FormspecParseException(true, os.str());
	}

	return v2s32(stoi(v[0]), stoi(v[1]));
}

std::map<std::string, std::string> FormspecElement::getParameters(size_t start) const {
	std::map<std::string, std::string> retval;
	for (size_t i = start; i < parts.size(); i++) {
		auto pair = split(parts[i], '=');
		if (pair.size() != 2 || pair[0].empty() || pair[1].empty()) {
			std::ostringstream os;
			os << "Invalid parameter at argument " << (start + 1);
			throw FormspecParseException(true, os.str());
		}

		retval[pair[0]] = pair[1];
	}

	return retval;
}
