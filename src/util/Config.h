#pragma once

#include "cpptoml.h"
#include <map>
#include <memory>
#include <string>

namespace rwr
{
class Config : public std::enable_shared_from_this<Config>
{
    std::string mFileName;

  protected:
    virtual void validateConfig(){};

    virtual void assignOptions(std::shared_ptr<cpptoml::table> group) = 0;

    bool assignPositiveInt(std::shared_ptr<cpptoml::table> group, std::string name, int& ret);
    bool assignString(std::shared_ptr<cpptoml::table> group, std::string name, std::string& ret);
    bool assignBool(std::shared_ptr<cpptoml::table> group, std::string name, bool& ret);
    bool assignDouble(std::shared_ptr<cpptoml::table> group, std::string name, double& ret);
    bool assignFloat(std::shared_ptr<cpptoml::table> group, std::string name, float& ret);

    // returns true if no strays
    bool checkStrays(std::shared_ptr<cpptoml::table> group, const std::vector<const char*>& optionList,
                     std::string& strayName);

  public:
    typedef std::shared_ptr<Config> pointer;

    void load(std::string const& filename);
    void reload();
};
} // namespace rwr
