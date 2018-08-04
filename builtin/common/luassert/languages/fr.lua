local s = require('say')

s:set_namespace("fr")

s:set("assertion.same.positive", "Objets supposes de meme nature attendus.\nArgument passe:\n%s\nAttendu:\n%s")
s:set("assertion.same.negative", "Objets supposes de natures differentes attendus.\nArgument passe:\n%s\nNon attendu:\n%s")

s:set("assertion.equals.positive", "Objets supposes etre de valeur egale attendus.\nArgument passe:\n%s\nAttendu:\n%s")
s:set("assertion.equals.negative", "Objets supposes etre de valeurs differentes attendu.\nArgument passe:\n%s\nNon attendu:\n%s")

s:set("assertion.unique.positive", "Objet suppose etre unique attendu:\n%s")
s:set("assertion.unique.negative", "Objet suppose ne pas etre unique attendu:\n%s")

s:set("assertion.error.positive", "Erreur supposee etre generee.")
s:set("assertion.error.negative", "Erreur non supposee etre generee.\n%s")

s:set("assertion.truthy.positive", "Assertion supposee etre vraie mais de valeur:\n%s")
s:set("assertion.truthy.negative", "Assertion supposee etre fausse mais de valeur:\n%s")

s:set("assertion.falsy.positive", "Assertion supposee etre fausse mais de valeur:\n%s")
s:set("assertion.falsy.negative", "Assertion supposee etre vraie mais de valeur:\n%s")
