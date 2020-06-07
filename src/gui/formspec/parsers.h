#pragma once

#include "FormspecElement.h"
#include "ParserState.h"

#include <map>
#include <functional>

using FormspecElementParser = std::function<void(ParserState &, const FormspecElement &)>;

void initParsers(std::map<std::string, FormspecElementParser> &parsers);
