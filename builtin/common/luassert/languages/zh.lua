local s = require('say')

s:set_namespace('zh')

s:set("assertion.same.positive", "希望对象应该相同.\n实际值:\n%s\n希望值:\n%s")
s:set("assertion.same.negative", "希望对象应该不相同.\n实际值:\n%s\n不希望与:\n%s\n相同")

s:set("assertion.equals.positive", "希望对象应该相等.\n实际值:\n%s\n希望值:\n%s")
s:set("assertion.equals.negative", "希望对象应该不相等.\n实际值:\n%s\n不希望等于:\n%s")

s:set("assertion.unique.positive", "希望对象是唯一的:\n%s")
s:set("assertion.unique.negative", "希望对象不是唯一的:\n%s")

s:set("assertion.error.positive", "希望有错误被抛出.")
s:set("assertion.error.negative", "希望没有错误被抛出.\n%s")

s:set("assertion.truthy.positive", "希望结果为真，但是实际为:\n%s")
s:set("assertion.truthy.negative", "希望结果不为真，但是实际为:\n%s")

s:set("assertion.falsy.positive", "希望结果为假，但是实际为:\n%s")
s:set("assertion.falsy.negative", "希望结果不为假，但是实际为:\n%s")

s:set("assertion.called.positive", "希望被调用%s次, 但实际被调用了%s次")
s:set("assertion.called.negative", "不希望正好被调用%s次, 但是正好被调用了那么多次.")

s:set("assertion.called_with.positive", "希望没有参数的调用函数")
s:set("assertion.called_with.negative", "希望有参数的调用函数")

-- errors
s:set("assertion.internal.argtolittle", "函数'%s'需要最少%s个参数, 实际有%s个参数\n")
s:set("assertion.internal.badargtype", "bad argument #%s: 函数'%s'需要一个%s作为参数, 实际为: %s\n")
