local s = require('say')

s:set_namespace("ru")

s:set("assertion.same.positive", "Ожидали одинаковые объекты.\nПередали:\n%s\nОжидали:\n%s")
s:set("assertion.same.negative", "Ожидали разные объекты.\nПередали:\n%s\nНе ожидали:\n%s")

s:set("assertion.equals.positive", "Ожидали эквивалентные объекты.\nПередали:\n%s\nОжидали:\n%s")
s:set("assertion.equals.negative", "Ожидали не эквивалентные объекты.\nПередали:\n%s\nНе ожидали:\n%s")

s:set("assertion.unique.positive", "Ожидали, что объект будет уникальным:\n%s")
s:set("assertion.unique.negative", "Ожидали, что объект не будет уникальным:\n%s")

s:set("assertion.error.positive", "Ожидали ошибку.")
s:set("assertion.error.negative", "Не ожидали ошибку.\n%s")

s:set("assertion.truthy.positive", "Ожидали true, но значние оказалось:\n%s")
s:set("assertion.truthy.negative", "Ожидали не true, но значние оказалось:\n%s")

s:set("assertion.falsy.positive", "Ожидали false, но значние оказалось:\n%s")
s:set("assertion.falsy.negative", "Ожидали не false, но значние оказалось:\n%s")
