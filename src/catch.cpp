// Minetest
// SPDX-License-Identifier: LGPL-2.1-or-later

#include "catch.h"

#include "log.h"

namespace Catch {
	std::ostream& cout() {
		return rawstream;
	}
	std::ostream& clog() {
		return rawstream;
	}
	std::ostream& cerr() {
		return rawstream;
	}
}
