Say
====

[![travis-ci status](https://secure.travis-ci.org/Olivine-Labs/say.png)](http://travis-ci.org/#!/Olivine-Labs/say/builds)

say is a simple string key/value store for i18n or ay other case where you
want namespaced strings.

Check out [busted](http://www.olivinelabs.com/busted) for
extended examples.

```lua
s = require("say")

s:set_namespace("en")

s:set('money', 'I have %s dollars')
s:set('wow', 'So much money!')

print(s('money', 1000)) -- I have 1000 dollars

s:set_namespace("fr") -- switch to french!
s:set('so_much_money', "Tant d'argent!")

print(s('wow')) -- Tant d'argent!
s:set_namespace("en")  -- switch back to english!
print(s('wow')) -- So much money!
```
