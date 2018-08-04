local assert = require('luassert.assert')

assert._COPYRIGHT   = "Copyright (c) 2012 Olivine Labs, LLC."
assert._DESCRIPTION = "Extends Lua's built-in assertions to provide additional tests and the ability to create your own."
assert._VERSION     = "Luassert 1.7.9"

-- load basic asserts
require('luassert.assertions')
require('luassert.modifiers')
require('luassert.matchers')
require('luassert.formatters')

-- load default language
require('luassert.languages.en')

return assert
