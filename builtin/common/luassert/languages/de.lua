local s = require('say')

s:set_namespace('de')

s:set("assertion.same.positive", "Erwarte gleiche Objekte.\nGegeben:\n%s\nErwartet:\n%s")
s:set("assertion.same.negative", "Erwarte ungleiche Objekte.\nGegeben:\n%s\nNicht erwartet:\n%s")

s:set("assertion.equals.positive", "Erwarte die selben Objekte.\nGegeben:\n%s\nErwartet:\n%s")
s:set("assertion.equals.negative", "Erwarte nicht die selben Objekte.\nGegeben:\n%s\nNicht erwartet:\n%s")

s:set("assertion.unique.positive", "Erwarte einzigartiges Objekt:\n%s")
s:set("assertion.unique.negative", "Erwarte nicht einzigartiges Objekt:\n%s")

s:set("assertion.error.positive", "Es wird ein Fehler erwartet.")
s:set("assertion.error.negative", "Es wird kein Fehler erwartet, aber folgender Fehler trat auf:\n%s")

s:set("assertion.truthy.positive", "Erwarte, dass der Wert 'wahr' (truthy) ist.\nGegeben:\n%s")
s:set("assertion.truthy.negative", "Erwarte, dass der Wert 'unwahr' ist (falsy).\nGegeben:\n%s")

s:set("assertion.falsy.positive", "Erwarte, dass der Wert 'unwahr' ist (falsy).\nGegeben:\n%s")
s:set("assertion.falsy.negative", "Erwarte, dass der Wert 'wahr' (truthy) ist.\nGegeben:\n%s")

s:set("assertion.called.positive", "Erwarte, dass die Funktion %s mal aufgerufen wird, anstatt %s mal.")
s:set("assertion.called.negative", "Erwarte, dass die Funktion nicht genau %s mal aufgerufen wird.")

s:set("assertion.called_with.positive", "Erwarte, dass die Funktion mit den gegebenen Parametern aufgerufen wird.")
s:set("assertion.called_with.negative", "Erwarte, dass die Funktion nicht mit den gegebenen Parametern aufgerufen wird.")

-- errors
s:set("assertion.internal.argtolittle", "Die Funktion '%s' erwartet mindestens %s Parameter, gegeben: %s")
s:set("assertion.internal.badargtype", "bad argument #%s: Die Funktion '%s' erwartet einen Parameter vom Typ %s, gegeben: %s")
