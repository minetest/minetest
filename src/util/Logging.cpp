// Copyright 2014 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "util/Logging.h"
#include <array>

/*
Levels:
    TRACE
    DEBUG
    FATAL
    ERROR
    WARNING
    INFO
*/

namespace rwr
{

namespace
{

constexpr std::array<const char*, 1> loggers = {"Work"};
}

el::Configurations Logging::gDefaultConf;

void Logging::setFmt(std::string const& peerID, bool timestamps)
{
    std::string datetime;
    if(timestamps)
    {
        datetime = "%datetime{%M-%dT%H:%m:%s.%g}";
    }
    std::string shortFmt = datetime + "" + peerID + " [%logger %level] %msg";
    std::string longFmt = shortFmt + " [%fbase:%line]";

    gDefaultConf.setGlobally(el::ConfigurationType::Format, shortFmt);
    gDefaultConf.set(el::Level::Error, el::ConfigurationType::Format, longFmt);
    gDefaultConf.set(el::Level::Trace, el::ConfigurationType::Format, longFmt);
    gDefaultConf.set(el::Level::Fatal, el::ConfigurationType::Format, longFmt);
    el::Loggers::reconfigureAllLoggers(gDefaultConf);
}

void Logging::init()
{
    for(auto const& logger : loggers)
    {
        el::Loggers::getLogger(logger);
    }

    gDefaultConf.setToDefault();
    gDefaultConf.setGlobally(el::ConfigurationType::ToStandardOutput, "true");
    gDefaultConf.setGlobally(el::ConfigurationType::ToFile, "false");
    setFmt("");
}

void Logging::registerPartion(std::string const& partition) { el::Loggers::getLogger(partition); }

void Logging::setLoggingToFile(std::string const& filename)
{
    gDefaultConf.setGlobally(el::ConfigurationType::ToFile, "true");
    gDefaultConf.setGlobally(el::ConfigurationType::Filename, filename);
    el::Loggers::reconfigureAllLoggers(gDefaultConf);
}

el::Level Logging::getLogLevel(std::string const& partition)
{
    el::Logger* logger = el::Loggers::getLogger(partition);
    if(logger->typedConfigurations()->enabled(el::Level::Trace))
        return el::Level::Trace;
    if(logger->typedConfigurations()->enabled(el::Level::Debug))
        return el::Level::Debug;
    if(logger->typedConfigurations()->enabled(el::Level::Info))
        return el::Level::Info;
    if(logger->typedConfigurations()->enabled(el::Level::Warning))
        return el::Level::Warning;
    if(logger->typedConfigurations()->enabled(el::Level::Error))
        return el::Level::Error;
    if(logger->typedConfigurations()->enabled(el::Level::Fatal))
        return el::Level::Fatal;
    return el::Level::Unknown;
}

bool Logging::logDebug(std::string const& partition)
{
    auto lev = Logging::getLogLevel(partition);
    return lev == el::Level::Debug || lev == el::Level::Trace;
}

bool Logging::logTrace(std::string const& partition)
{
    auto lev = Logging::getLogLevel(partition);
    return lev == el::Level::Trace;
}

// Trace < Debug < Info < Warning < Error < Fatal < None
void Logging::setLogLevel(el::Level level, const char* partition)
{
    el::Configurations config = gDefaultConf;

    if(level == el::Level::Debug)
        config.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
    else if(level == el::Level::Info)
    {
        config.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");
    }
    else if(level == el::Level::Warning)
    {
        config.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Info, el::ConfigurationType::Enabled, "false");
    }
    else if(level == el::Level::Error)
    {
        config.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Info, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Warning, el::ConfigurationType::Enabled, "false");
    }
    else if(level == el::Level::Fatal)
    {
        config.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Info, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Warning, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Error, el::ConfigurationType::Enabled, "false");
    }
    else if(level == el::Level::Unknown)
    {
        config.set(el::Level::Trace, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Debug, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Info, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Warning, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Error, el::ConfigurationType::Enabled, "false");
        config.set(el::Level::Fatal, el::ConfigurationType::Enabled, "false");
    }

    if(partition)
        el::Loggers::reconfigureLogger(partition, config);
    else
        el::Loggers::reconfigureAllLoggers(config);
}

std::string Logging::getStringFromLL(el::Level level)
{
    switch(level)
    {
    case el::Level::Global:
        return "Global";
    case el::Level::Trace:
        return "Trace";
    case el::Level::Debug:
        return "Debug";
    case el::Level::Fatal:
        return "Fatal";
    case el::Level::Error:
        return "Error";
    case el::Level::Warning:
        return "Warning";
    case el::Level::Verbose:
        return "Verbose";
    case el::Level::Info:
        return "Info";
    case el::Level::Unknown:
        return "Unknown";
    }
    return "????";
}

// default "info" if unrecognized
el::Level Logging::getLLfromString(std::string const& levelName)
{
    if(iequals(levelName, "trace"))
    {
        return el::Level::Trace;
    }

    if(iequals(levelName, "debug"))
    {
        return el::Level::Debug;
    }

    if(iequals(levelName, "warning"))
    {
        return el::Level::Warning;
    }

    if(iequals(levelName, "fatal"))
    {
        return el::Level::Fatal;
    }

    if(iequals(levelName, "error"))
    {
        return el::Level::Error;
    }

    if(iequals(levelName, "none"))
    {
        return el::Level::Unknown;
    }

    return el::Level::Info;
}

void Logging::rotate()
{
    el::Loggers::getLogger("default")->reconfigure();
    for(auto const& logger : loggers)
    {
        el::Loggers::getLogger(logger)->reconfigure();
    }
}

bool iequals(std::string const& a, std::string const& b)
{
    size_t sz = a.size();
    if(b.size() != sz)
        return false;
    for(size_t i = 0; i < sz; ++i)
        if(tolower(a[i]) != tolower(b[i]))
            return false;
    return true;
}
} // namespace rwr
