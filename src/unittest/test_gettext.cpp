#include "test.h"
#include "porting.h"
#include "gettext.h"

class TestGettext : public TestBase
{
public:
	TestGettext() {
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
	TEST(testFmtgettext);
}

void TestGettext::testFmtgettext()
{
  std::string buf = fmtgettext("Viewing range changed to %d",  12);
  UASSERTEQ(std::string, buf, "Viewing range changed to 12");
  buf = fmtgettext(
    "You are about to join this server with the name \"%s\" for the "
    "first time.\n"
    "If you proceed, a new account using your credentials will be "
    "created on this server.\n"
    "Please retype your password and click 'Register and Join' to "
    "confirm account creation, or click 'Cancel' to abort."
    ,  "A");
  UASSERTEQ(std::string, buf,
    "You are about to join this server with the name \"A\" for the "
    "first time.\n"
    "If you proceed, a new account using your credentials will be "
    "created on this server.\n"
    "Please retype your password and click 'Register and Join' to "
    "confirm account creation, or click 'Cancel' to abort."
  );
}
