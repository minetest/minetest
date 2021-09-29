#include "test.h"
#include "porting.h"
#include "gettext.h"

class TestGettext : public TestBase
{
public:
	TestGettext() {
		// init_gettext(porting::path_locale.c_str(), "en", 0, nullptr);
		// setenv("LANGUAGE", "en", 1);
		// setlocale(LC_ALL, "");

		TestManager::registerTestModule(this);
		}

	const char *getName() { return "TestGettext"; }

	void runTests(IGameDef *gamedef);

	void testSnfmtgettext();
	void testFmtgettext();
};

static TestGettext g_test_instance;

void TestGettext::runTests(IGameDef *gamedef)
{
	TEST(testSnfmtgettext);
	TEST(testFmtgettext);
}

void TestGettext::testSnfmtgettext()
{
  std::string buf = "";
  buf.resize(8);
  int len = snfmtgettext(&buf[0], 8, "Viewing range changed to %d", 12);
  UASSERTEQ(int, len, 27);
  buf.pop_back();
  UASSERTEQ(std::string, buf, "Viewing");

  buf.resize(28);
  len = snfmtgettext(&buf[0], 28, "Viewing range changed to %d", 12);
  UASSERTEQ(int, len, 27);
  buf.pop_back();
  UASSERTEQ(std::string, buf, "Viewing range changed to 12");
}

void TestGettext::testFmtgettext()
{
  std::string buf = fmtgettext("Viewing range changed to %d", 8,  12);
  UASSERTEQ(std::string, buf, "Viewing range changed to 12");
  buf = fmtgettext("Viewing range changed to %d", 0,  12);
  UASSERTEQ(std::string, (char*)&buf[0], "Viewing range changed to 12");
}
