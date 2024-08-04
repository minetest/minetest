#include "util/langcode.h"
#include "catch.h"

TEST_CASE("test langcode")
{
	SECTION("test language list")
	{
		CHECK(get_tr_language("de_DE.UTF-8:fr") == "de_DE:de:fr");
		CHECK(get_tr_language("de_DE:fr:de_CH") == "de_DE:fr:de_CH:de");
		CHECK(get_tr_language("de_DE:fr:de_CH:en:de:de_AT") == "de_DE:fr:de_CH:en:de:de_AT");
		CHECK(get_tr_language(".UTF-8:de:.ISO-8859-1:fr:.GB2312") == "de:fr");
	}
}
