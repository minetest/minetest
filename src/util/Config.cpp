#include "Config.h"
#include "util/Logging.h"

#include <functional>
#include <sstream>

namespace rwr
{
bool Config::assignPositiveInt(std::shared_ptr<cpptoml::table> group, std::string name, int& ret)
{
    auto val = group->get_as<int64_t>(name);
    if(val)
    {
        ret = (int)(*val);
        return true;
    }
    return false;
}

bool Config::assignDouble(std::shared_ptr<cpptoml::table> group, std::string name, double& ret)
{
    auto val = group->get_as<double>(name);
    if(val)
    {
        ret = *val;
        return true;
    }
    return false;
}

bool Config::assignFloat(std::shared_ptr<cpptoml::table> group, std::string name, float& ret)
{
    auto val = group->get_as<double>(name);
    if(val)
    {
        ret = (float)(*val);
        return true;
    }
    return false;
}

bool Config::assignString(std::shared_ptr<cpptoml::table> group, std::string name, std::string& ret)
{
    auto val = group->get_as<std::string>(name);
    if(val)
    {
        ret = *val;
        return true;
    }
    return false;
}

bool Config::assignBool(std::shared_ptr<cpptoml::table> group, std::string name, bool& ret)
{
    auto val = group->get_as<bool>(name);
    if(val)
    {
        ret = *val;
        return true;
    }
    return false;
}

// returns true if no strays
bool Config::checkStrays(std::shared_ptr<cpptoml::table> group, const std::vector<const char*>& optionList,
                         std::string& strayName)
{
    for(const auto& table : *group)
    {
        bool found = false;
        for(const char* opt : optionList)
        {
            if(opt == table.first)
            {
                found = true;
                break;
            }
        }
        if(!found)
        {
            strayName = table.first;
            return false;
        }
    }
    return true;
}

void Config::reload() { load(mFileName); }

void Config::load(std::string const& filename)
{
    mFileName = filename;
    LOG(DEBUG) << "Loading config from: " << filename;
    try
    {
        std::shared_ptr<cpptoml::table> g;
        if(filename == "-")
        {
            cpptoml::parser p(std::cin);
            g = p.parse();
        }
        else
        {
            g = cpptoml::parse_file(filename);
        }
        // cpptoml returns the items in non-deterministic order
        // so we need to process items that are potential dependencies first

        assignOptions(g);
        validateConfig();
    }
    catch(std::exception& ex)
    {
        std::string err("Failed to parse '");
        err += filename;
        err += "' :";
        err += ex.what();
        LOG(ERROR) << err;
        throw std::invalid_argument(err);
    }
}
} // namespace rwr
