#pragma once

// note this file should usually be last in the #include list for windows.h issues

#include "easylogging++.h"

namespace rwr
{
class Logging
{
    static el::Configurations gDefaultConf;

  public:
    static void init();
    static void setFmt(std::string const& peerID, bool timestamps = true);
    static void setLoggingToFile(std::string const& filename);
    static void setLogLevel(el::Level level, const char* partition);
    static el::Level getLLfromString(std::string const& levelName);
    static el::Level getLogLevel(std::string const& partition);
    static std::string getStringFromLL(el::Level);
    static bool logDebug(std::string const& partition);
    static bool logTrace(std::string const& partition);
    static void rotate();

    static void registerPartion(std::string const& partition);
};

bool iequals(std::string const& a, std::string const& b);
} // namespace rwr
