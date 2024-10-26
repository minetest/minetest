// Minetest
// SPDX-License-Identifier: LGPL-2.1-or-later

// We want to have catch write to rawstream (stderr) instead of stdout.
// This should be included instead of <catch_amalgamated.hpp>
// to patch the output streams accordingly.
#define CATCH_CONFIG_NOSTDOUT
#include <catch_amalgamated.hpp>

namespace Catch {
	std::ostream& cout();
	std::ostream& clog();
	std::ostream& cerr();
}
