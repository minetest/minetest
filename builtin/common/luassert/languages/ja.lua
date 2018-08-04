local s = require('say')

s:set_namespace('ja')

s:set("assertion.same.positive", "オブジェクトの内容が同一であることが期待されています。\n実際の値:\n%s\n期待されている値:\n%s")
s:set("assertion.same.negative", "オブジェクトの内容が同一でないことが期待されています。\n実際の値:\n%s\n期待されていない値:\n%s")

s:set("assertion.equals.positive", "オブジェクトが同一であることが期待されています。\n実際の値:\n%s\n期待されている値:\n%s")
s:set("assertion.equals.negative", "オブジェクトが同一でないことが期待されています。\n実際の値:\n%s\n期待されていない値:\n%s")

s:set("assertion.unique.positive", "オブジェクトがユニークであることが期待されています。:\n%s")
s:set("assertion.unique.negative", "オブジェクトがユニークでないことが期待されています。:\n%s")

s:set("assertion.error.positive", "エラーが発生することが期待されています。")
s:set("assertion.error.negative", "エラーが発生しないことが期待されています。")

s:set("assertion.truthy.positive", "真であることが期待されていますが、値は:\n%s")
s:set("assertion.truthy.negative", "真でないことが期待されていますが、値は:\n%s")

s:set("assertion.falsy.positive", "偽であることが期待されていますが、値は:\n%s")
s:set("assertion.falsy.negative", "偽でないことが期待されていますが、値は:\n%s")

s:set("assertion.called.positive", "回呼ばれることを期待されていますが、実際には%s回呼ばれています。")
s:set("assertion.called.negative", "回呼ばれることを期待されていますが、実際には%s回呼ばれています。")

s:set("assertion.called_with.positive", "関数が期待されている引数で呼ばれていません")
s:set("assertion.called_with.negative", "関数が期待されている引数で呼ばれています")

s:set("assertion.returned_arguments.positive", "期待されている返り値の数は%sですが、実際の返り値の数は%sです。")
s:set("assertion.returned_arguments.negative", "期待されていない返り値の数は%sですが、実際の返り値の数は%sです。")

-- errors
s:set("assertion.internal.argtolittle", "関数には最低%s個の引数が必要ですが、実際の引数の数は: %s")
s:set("assertion.internal.badargtype", "bad argument #%s: 関数には%s個の引数が必要ですが、実際に引数の数は: %s")
