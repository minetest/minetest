local s = require('say')

s:set_namespace('nl')

s:set("assertion.same.positive", "Verwachtte objecten die vergelijkbaar zijn.\nAangeboden:\n%s\nVerwachtte:\n%s")
s:set("assertion.same.negative", "Verwachtte objecten die niet vergelijkbaar zijn.\nAangeboden:\n%s\nVerwachtte niet:\n%s")

s:set("assertion.equals.positive", "Verwachtte objecten die hetzelfde zijn.\nAangeboden:\n%s\nVerwachtte:\n%s")
s:set("assertion.equals.negative", "Verwachtte objecten die niet hetzelfde zijn.\nAangeboden:\n%s\nVerwachtte niet:\n%s")

s:set("assertion.unique.positive", "Verwachtte objecten die uniek zijn:\n%s")
s:set("assertion.unique.negative", "Verwachtte objecten die niet uniek zijn:\n%s")

s:set("assertion.error.positive", "Verwachtte een foutmelding.")
s:set("assertion.error.negative", "Verwachtte geen foutmelding.\n%s")

s:set("assertion.truthy.positive", "Verwachtte een 'warige' (thruthy) waarde, maar was:\n%s")
s:set("assertion.truthy.negative", "Verwachtte een niet 'warige' (thruthy) waarde, maar was:\n%s")

s:set("assertion.falsy.positive", "Verwachtte een 'onwarige' (falsy) waarde, maar was:\n%s")
s:set("assertion.falsy.negative", "Verwachtte een niet 'onwarige' (falsy) waarde, maar was:\n%s")

-- errors
s:set("assertion.internal.argtolittle", "de '%s' functie verwacht minimaal %s parameters, maar kreeg er: %s")
s:set("assertion.internal.badargtype", "bad argument #%s: de '%s' functie verwacht een %s als parameter, maar kreeg een: %s")
