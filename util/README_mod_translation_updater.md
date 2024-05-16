# `mod_translation_updater.py`—Minetest Mod Translation Updater

This Python script is intended for use with localized Minetest mods, i.e., mods that use
`*.tr` and contain translatable strings of the form `S("This string can be translated")`.
It extracts the strings from the mod's source code and updates the localization files
accordingly. It can also be used to update the `*.tr` files in Minetest's `builtin` component.

## Preparing your source code

This script makes assumptions about your source code. Before it is usable, you first have
to prepare your source code accordingly.

### Choosing the textdomain name

It is recommended to set the textdomain name (for `minetest.get_translator`) to be identical
of the mod name as the script will automatically detect it. If the textdomain name differs,
you may have to manually change the `# textdomain:` line of newly generated files.

**Note:** In each `*.tr` file, there **must** be only one textdomain. Multiple textdomains in
the same file are not supported by this script and any additional textdomain line will be
removed.

### Defining the helper functions

In any source code file with translatable strings, you have to manually define helper
functions at the top with something like `local S = minetest.get_translator("<textdomain>")`.
Optionally, you can also define additional helper functions `FS`, `NS` and `NFS` if needed.

Here is the list of all recognized function names. All functions return a string.

* `S`: Returns translation of input. See Minetest's `lua_api.md`. You should always have at
       least this function defined.
* `NS`: Returns the input. Useful to make a string visible to the script without actually
        translating it here.
* `FS`: Same as `S`, but returns a formspec-escaped version of the translation of the input.
        Supported for convenience.
* `NFS`: Returns a formspec-escaped version of the input, but not translated.
         Supported for convenience.

Here is the boilerplate code you have to add at the top of your source code file:

    local S = minetest.get_translator("<textdomain>")
    local NS = function(s) return s end
    local FS = function(...) return minetest.formspec_escape(S(...)) end
    local NFS = function(s) return minetest.formspec_escape(s) end

Replace `<textdomain>` above and optionally delete `NS`, `FS` and/or `NFS` if you don't need
them.

### Preparing the strings

This script can detect translatable strings of the notations listed below.
Additional function arguments followed after a literal string are ignored.

* `S("literal")`: one literal string enclosed by the delimiters
  `"..."`, `'...'` or `[[...]]`
* `S("foo " .. 'bar ' .. "baz")`: concatenation of multiple literal strings. Line
  breaks are accepted.

The `S` may also be `NS`, `FS` and `NFS` (see above).

Undetectable notations:

* `S"literal"`: omitted function brackets
* `S(variable)`: requires the use of `NS`. See example below.
* `S("literal " .. variable)`: non-static content.
  Use placeholders (`@1`, ...) for variable text.
* Any literal string concatenation using `[[...]]`

### A minimal example

This minimal code example sends "Hello world!" to all players, but translated according to
each player's language:

    local S = minetest.get_translator("example")
    minetest.chat_send_all(S("Hello world!"))

### How to use `NS`

The reason why `NS` exists is for cases like this: Sometimes, you want to define a list of
strings to they can be later output in a function. Like so:

    local fruit = { "Apple", "Orange", "Pear" }
    local function return_fruit(fruit_id)
       return fruit[fruit_id]
    end

If you want to translate the fruit names when `return_fruit` is run, but have the
*untranslated* fruit names in the `fruit` table stored, this is where `NS` will help.
It will show the script the string without Minetest translating it. The script could be made
translatable like this:

    local fruit = { NS("Apple"), NS("Orange"), NS("Pear") }
    local function return_fruit(fruit_id)
       return S(fruit[fruit_id])
    end

## How to run the script

First, change the working directory to the directory of the mod you want the files to be
updated. From this directory, run the script.

When you run the script, it will update the `template.txt` and any `*.tr` files present
in that mod's `/locale` folder. If the `/locale` folder or `template.txt` file don't
exist yet, they will be created.

This script will also work in the root directory of a modpack. It will run on each mod
inside the modpack in that situation. Alternatively, you can run the script to update
the files of all mods in subdirectories with the `-r` option, which is useful to update
the locale files in an entire game.

It has the following command line options:

    mod_translation_updater.py [OPTIONS] [PATHS...]

    --help, -h: prints this help message
    --recursive, -r: run on all subfolders of paths given
    --old-file, -o: create copies of files before updating them, named `<FILE NAME>.old`
    --break-long-lines, -b: add extra line-breaks before and after long strings
    --print-source, -p: add comments denoting the source file
    --verbose, -v: add output information
    --truncate-unused, -t: delete unused strings from files

## Script output

This section explains how the output of this script works, roughly. This script aims to make
the output more or less stable, i.e. given identical source files and arguments, the script
should produce the same output.

### Textdomain

The script will add (if not already present) a `# textdomain: <modname>` at the top, where
`<modname>` is identical to the mod directory name. If a `# textdomain` already exists, it
will be moved to the top, with the textdomain name being left intact (even if it differs
from the mod name).

**Note:** If there are multiple `# textdomain:` lines in the file, all of them except the
first one will be deleted. This script only supports one textdomain per `*.tr` file.

### Strings

The order of the strings is deterministic and follows certain rules: First, all strings are
grouped by the source `*.lua` file. The files are loaded in alphabetical order. In case of
subdirectories, the mod's root directory takes precedence, then the directories are traversed
in top-down alphabetical order. Second, within each file, the strings are then inserted in
the same order as they appear in the source code.

If a string appears multiple times in the source code, the string will be added when it was
first found only.

Don't bother to manually organize the order of the lines in the file yourself because the
script will just reorder everything.

If the mod's source changes in such a way that a line with an existing translation or comment
is no longer present, and `--truncate-unused` or `-t` are *not* provided as arguments, the
unused line will be moved to the bottom of the translation file under a special comment:

    ##### not used anymore #####

This allows for old translations and comments to be reused with new lines where appropriate.
This script doesn't attempt "fuzzy" matching of old strings to new, so even a single change
of punctuation or spelling will put strings into the "not used anymore" section and require
manual re-association with the new string.

### Comments

The script will preserve any comments in an existing `template.txt` or the various `*.tr`
files, associating them with the line that follows them. So for example:

    # This comment pertains to Some Text
    Some text=
    
    # Multi-line comments
    # are also supported
    Text as well=

The script will also propagate comments from an existing `template.txt` to all `*.tr`
files and write it above existing comments (if exist).

There are also a couple of special comments that this script gives special treatment to.

#### Source file comments

If `--print-source` or `-p` is provided as option, the script will insert comments to show
from which file or files each string has come from.
This is the syntax of such a comment:

    ##[ file.lua ]##

This comment means that all lines following it belong to the file `file.lua`. In the special
case the same string was found in multiple files, multiple file name comments will be used in
row, like so:

    ##[ file1.lua ]##
    ##[ file2.lua ]##
    ##[ file3.lua ]##
    example=Beispiel

This means the string "example" was found in the files `file1.lua`, `file2.lua` and
`file3.lua`.

If the print source option is not provided, these comments will disappear.

Note that all comments of the form `##[something]##` will be treated as "source file" comments
so they may be moved, changed or removed by the script at will.

#### "not used anymore" section

By default, the exact comment `##### not used anymore #####` will be automatically added to
mark the beginning of a section where old/unused strings will go. Leave the exact wording of
this comment intact so this line can be moved (or removed) properly in subsequent runs.

## Updating `builtin`

To update the `builtin` component of Minetest, change the working directory to `builtin` of
the Minetest source code repository, then run this script from there.
