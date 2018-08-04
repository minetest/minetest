local s = require('say')

s:set_namespace("ar")

s:set("assertion.same.positive", "تُوُقِّعَ تَماثُلُ الكائِنات.\nتَمَّ إدخال:\n %s.\nبَينَما كانَ مِن المُتَوقَّع:\n %s.")
s:set("assertion.same.negative", "تُوُقِّعَ إختِلافُ الكائِنات.\nتَمَّ إدخال:\n %s.\nبَينَما كانَ مِن غَيرِ المُتَوقَّع:\n %s.")

s:set("assertion.equals.positive", "تُوُقِّعَ أن تَتَساوىْ الكائِنات.\nتمَّ إِدخال:\n %s.\nبَينَما كانَ من المُتَوقَّع:\n %s.")
s:set("assertion.equals.negative", "تُوُقِّعَ ألّا تَتَساوىْ الكائِنات.\nتمَّ إِدخال:\n %s.\nبَينَما كانَ مِن غير المُتًوقَّع:\n %s.")

s:set("assertion.unique.positive", "تُوُقِّعَ أَنْ يَكونَ الكائِنٌ فَريد: \n%s")
s:set("assertion.unique.negative", "تُوُقِّعَ أنْ يَكونَ الكائِنٌ غَيرَ فَريد: \n%s")

s:set("assertion.error.positive", "تُوُقِّعَ إصدارُ خطأْ.")
s:set("assertion.error.negative", "تُوُقِّعَ عدم إصدارِ خطأ.")

s:set("assertion.truthy.positive", "تُوُقِّعَت قيمةٌ صَحيحة، بينما كانت: \n%s")
s:set("assertion.truthy.negative", "تُوُقِّعَت قيمةٌ غيرُ صَحيحة، بينما كانت: \n%s")

s:set("assertion.falsy.positive", "تُوُقِّعَت قيمةٌ خاطِئة، بَينَما كانت: \n%s")
s:set("assertion.falsy.negative", "تُوُقِّعَت قيمةٌ غيرُ خاطِئة، بَينَما كانت: \n%s")
