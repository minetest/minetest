#include <irrString.h>
#include <cstring>
#include <clocale>
#include <vector>
#include "test_helper.h"

using namespace irr;
using namespace irr::core;

#define CMPSTR(a, b) (!strcmp(a, b))
#define UASSERTSTR(actual, expected) UASSERTCMP(CMPSTR, actual.c_str(), expected)

static void test_basics()
{
	// ctor
	stringc s;
	UASSERTEQ(s.c_str()[0], '\0');
	s = stringc(0.1234567);
	UASSERTSTR(s, "0.123457");
	s = stringc(0x1p+53);
	UASSERTSTR(s, "9007199254740992.000000");
	s = stringc(static_cast<int>(-102400));
	UASSERTSTR(s, "-102400");
	s = stringc(static_cast<unsigned int>(102400));
	UASSERTSTR(s, "102400");
	s = stringc(static_cast<long>(-1024000));
	UASSERTSTR(s, "-1024000");
	s = stringc(static_cast<unsigned long>(1024000));
	UASSERTSTR(s, "1024000");
	s = stringc("YESno", 3);
	UASSERTSTR(s, "YES");
	s = stringc(L"test", 4);
	UASSERTSTR(s, "test");
	s = stringc("Hello World!");
	UASSERTSTR(s, "Hello World!");
	// operator=
	s = stringw(L"abcdef");
	UASSERTSTR(s, "abcdef");
	s = L"abcdef";
	UASSERTSTR(s, "abcdef");
	s = static_cast<const char *>(nullptr);
	UASSERTSTR(s, "");
	// operator+
	s = s + stringc("foo");
	UASSERTSTR(s, "foo");
	s = s + L"bar";
	UASSERTSTR(s, "foobar");
	// the rest
	s = "f";
	UASSERTEQ(s[0], 'f');
	const auto &sref = s;
	UASSERTEQ(sref[0], 'f');
	UASSERT(sref == "f");
	UASSERT(sref == stringc("f"));
	s = "a";
	UASSERT(sref < stringc("aa"));
	UASSERT(sref < stringc("b"));
	UASSERT(stringc("Z") < sref);
	UASSERT(!(sref < stringc("a")));
	UASSERT(sref.lower_ignore_case("AA"));
	UASSERT(sref.lower_ignore_case("B"));
	UASSERT(!sref.lower_ignore_case("A"));
	s = "dog";
	UASSERT(sref != "cat");
	UASSERT(sref != stringc("cat"));
}

static void test_methods()
{
	stringc s;
	const auto &sref = s;
	s = "irrlicht";
	UASSERTEQ(sref.size(), 8);
	UASSERT(!sref.empty());
	s.clear();
	UASSERTEQ(sref.size(), 0);
	UASSERT(sref.empty());
	UASSERT(sref[0] == 0);
	s = "\tAz#`";
	s.make_lower();
	UASSERTSTR(s, "\taz#`");
	s.make_upper();
	UASSERTSTR(s, "\tAZ#`");
	UASSERT(sref.equals_ignore_case("\taz#`"));
	UASSERT(sref.equals_substring_ignore_case("Z#`", 2));
	s = "irrlicht";
	UASSERT(sref.equalsn(stringc("irr"), 3));
	UASSERT(sref.equalsn("irr", 3));
	s = "fo";
	s.append('o');
	UASSERTSTR(s, "foo");
	s.append("bar", 1);
	UASSERTSTR(s, "foob");
	s.append("ar", 999999);
	UASSERTSTR(s, "foobar");
	s = "nyan";
	s.append(stringc("cat"));
	UASSERTSTR(s, "nyancat");
	s.append(stringc("sam"), 1);
	UASSERTSTR(s, "nyancats");
	s = "fbar";
	s.insert(1, "ooXX", 2);
	UASSERTSTR(s, "foobar");
	UASSERTEQ(sref.findFirst('o'), 1);
	UASSERTEQ(sref.findFirst('X'), -1);
	UASSERTEQ(sref.findFirstChar("abff", 2), 3);
	UASSERTEQ(sref.findFirstCharNotInList("fobb", 2), 3);
	UASSERTEQ(sref.findLast('o'), 2);
	UASSERTEQ(sref.findLast('X'), -1);
	UASSERTEQ(sref.findLastChar("abrr", 2), 4);
	UASSERTEQ(sref.findLastCharNotInList("rabb", 2), 3);
	UASSERTEQ(sref.findNext('o', 2), 2);
	UASSERTEQ(sref.findLast('o', 1), 1);
	s = "ob-oob";
	UASSERTEQ(sref.find("ob", 1), 4);
	UASSERTEQ(sref.find("ob"), 0);
	UASSERTEQ(sref.find("?"), -1);
	s = "HOMEOWNER";
	stringc s2 = sref.subString(2, 4);
	UASSERTSTR(s2, "MEOW");
	s2 = sref.subString(2, 4, true);
	UASSERTSTR(s2, "meow");
	s = "land";
	s.replace('l', 's');
	UASSERTSTR(s, "sand");
	s = ">dog<";
	s.replace("dog", "cat");
	UASSERTSTR(s, ">cat<");
	s.replace("cat", "horse");
	UASSERTSTR(s, ">horse<");
	s.replace("horse", "gnu");
	UASSERTSTR(s, ">gnu<");
	s = " h e l p ";
	s.remove(' ');
	UASSERTSTR(s, "help");
	s.remove("el");
	UASSERTSTR(s, "hp");
	s = "irrlicht";
	s.removeChars("it");
	UASSERTSTR(s, "rrlch");
	s = "\r\nfoo bar ";
	s.trim();
	UASSERTSTR(s, "foo bar");
	s = "foxo";
	s.erase(2);
	UASSERTSTR(s, "foo");
	s = "a";
	s.append('\0');
	s.append('b');
	UASSERTEQ(s.size(), 3);
	s.validate();
	UASSERTEQ(s.size(), 1);
	UASSERTEQ(s.lastChar(), 'a');
	std::vector<stringc> res;
	s = "a,,b,c";
	s.split(res, ",aa", 1, true, false);
	UASSERTEQ(res.size(), 3);
	UASSERTSTR(res[0], "a");
	UASSERTSTR(res[2], "c");
	res.clear();
	s.split(res, ",", 1, false, true);
	UASSERTEQ(res.size(), 7);
	UASSERTSTR(res[0], "a");
	UASSERTSTR(res[2], "");
	for (int i = 0; i < 3; i++)
		UASSERTSTR(res[2 * i + 1], ",");
}

static void test_conv()
{
	// locale-independent

	stringw out;
	utf8ToWString(out, "†††");
	UASSERTEQ(out.size(), 3);
	for (int i = 0; i < 3; i++)
		UASSERTEQ(static_cast<u16>(out[i]), 0x2020);

	stringc out2;
	wStringToUTF8(out2, L"†††");
	UASSERTEQ(out2.size(), 9);
	for (int i = 0; i < 3; i++) {
		UASSERTEQ(static_cast<u8>(out2[3 * i]), 0xe2);
		UASSERTEQ(static_cast<u8>(out2[3 * i + 1]), 0x80);
		UASSERTEQ(static_cast<u8>(out2[3 * i + 2]), 0xa0);
	}

	// locale-dependent
	if (!setlocale(LC_CTYPE, "C.UTF-8"))
		setlocale(LC_CTYPE, "UTF-8"); // macOS

	stringw out3;
	multibyteToWString(out3, "†††");
	UASSERTEQ(out3.size(), 3);
	for (int i = 0; i < 3; i++)
		UASSERTEQ(static_cast<u16>(out3[i]), 0x2020);
}

void test_irr_string()
{
	test_basics();
	test_methods();
	test_conv();
	std::cout << "    test_irr_string PASSED" << std::endl;
}
