local s = require('say')

s:set_namespace("ua")

s:set("assertion.same.positive", "Очікували однакові обєкти.\nПередали:\n%s\nОчікували:\n%s")
s:set("assertion.same.negative", "Очікували різні обєкти.\nПередали:\n%s\nНе очікували:\n%s")

s:set("assertion.equals.positive", "Очікували еквівалентні обєкти.\nПередали:\n%s\nОчікували:\n%s")
s:set("assertion.equals.negative", "Очікували не еквівалентні обєкти.\nПередали:\n%s\nНе очікували:\n%s")

s:set("assertion.unique.positive", "Очікували, що обєкт буде унікальним:\n%s")
s:set("assertion.unique.negative", "Очікували, що обєкт не буде унікальним:\n%s")

s:set("assertion.error.positive", "Очікували помилку.")
s:set("assertion.error.negative", "Не очікували помилку.\n%s")

s:set("assertion.truthy.positive", "Очікували true, проте значння виявилось:\n%s")
s:set("assertion.truthy.negative", "Очікували не true, проте значння виявилось:\n%s")

s:set("assertion.falsy.positive", "Очікували false, проте значння виявилось:\n%s")
s:set("assertion.falsy.negative", "Очікували не false, проте значння виявилось:\n%s")
