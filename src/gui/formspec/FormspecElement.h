#pragma once

#include <vector>
#include "util/string.h"

class FormspecElement {
	const std::vector<std::string> parts;

public:
	/** Create a formspec element parser
	 *
	 * @param source Source inside `type[]`
	 */
	FormspecElement(const std::string &source):
			parts(split(source, ';'))
	{}

	inline int getNumberOfParts() const {
		return parts.size();
	}

	void checkLength(bool relaxed, size_t expectedSize) const;

	const std::string &getString(size_t idx) const {
		return parts[idx];
	}

	int getInt(size_t idx) const {
		return stoi(getString(idx));
	}

	float getFloat(size_t idx) const {
		return stof(getString(idx));
	}

	v2f32 getVector(size_t idx) const;

	std::map<std::string, std::string> getParameters(size_t start) const;
};
