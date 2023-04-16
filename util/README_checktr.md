# `checktr.py`

`checktr.py` is an utility script for Minetest to check mod or builtin translation files
(`*.tr` and `template.txt` files) in the working directory and all its child directories.
It will look for common syntax errors and report everything it finds on the console.

## How to use

You need to have Python 3 installed first.

Run this script as `./checktr.py <PATH>` where `<PATH>` is the path you want to check.
It will check all translation files in this directory and sub-directories.

There are a few extra parameters, invoke `./checktr.py --help` for a list.

The script will do its tests and print the test results on the console.

The script may report errors and warnings about the tested files. It will first write the type
of message (error or warning), then the file name and line number (separated by a colon,
e.g. `./example_mod/locale/example_mod.de.tr:15` is line 15 in `example_mod.de.tr`), then
the actual message. If a message applies to the whole file, no line number is shown.

An error is always something against the rules and must be fixed.

A warning is something that isn’t technically against the rules but it is probably a mistake anyway.
Check this one manually.

A summary is always written at the end, indicating how many files had errors and warnings.

## Syntax rules for translation files

This script uses `lua_api.txt` of Minetest 5.6.0 as a reference for the correct syntax of `*.tr` files.

Additionally, all files are expected to be UTF-8-encoded.

Additionally, `template.txt` files are also checked as if they were `*.tr` files,
except all translations are expected to be empty.

(Note: By convention, a `template.txt` file is a generated file that has the same syntax of a `*.tr` file
so it can be copied to create a new translation file more easily.)

## Supported errors and warnings

These things are considered to be an error:

* Invalid use of uppercase for `*.tr` file name suffix
* Missing language code in `*.tr` file name
* `*.tr` file is stored in a directory not named `locale`
* Invalid escape codes (e.g. unescaped `@` or `=` sign)
* Invalid line types (lines that are neither empty, a comment, a textdomain or a translation)
* Missing placeholder (`@1`, etc.) in translation that is present in source string
* Placeholder in translation that does not exist in source string
* Incorrect order of placeholders (e.g. `@2`, then `@1`) in the source string
* Repeated placeholder in the source string
* Empty textdomain
* Non-empty translation in `template.txt`

Warnings are shown for:

* No prefix in `*.tr` file name (e.g. the file name is literally `.de.tr`)
* Missing space between `#` and `textdomain`
* Empty source strings
* Missing textdomain in file
* `template.txt` file is stored in a directory not named `locale`
