#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Script to generate the template file and update the translation files.
# Copy the script into the mod or modpack root folder and run it there.
#
# Copyright (C) 2019 Joachim Stolberg, 2020 FaceDeer, 2020 Louis Royer
# LGPLv2.1+
#
# See https://github.com/minetest-tools/update_translations for
# potential future updates to this script.

from __future__ import print_function
import os, fnmatch, re, shutil, errno
from sys import argv as _argv
from sys import stderr as _stderr

# Running params
params = {"recursive": False,
    "help": False,
    "mods": False,
    "verbose": False,
    "folders": [],
    "old-file": False,
    "break-long-lines": False,
    "sort": False,
    "print-source": False,
    "truncate-unused": False,
}
# Available CLI options
options = {"recursive": ['--recursive', '-r'],
    "help": ['--help', '-h'],
    "mods": ['--installed-mods', '-m'],
    "verbose": ['--verbose', '-v'],
    "old-file": ['--old-file', '-o'],
    "break-long-lines": ['--break-long-lines', '-b'],
    "sort": ['--sort', '-s'],
    "print-source": ['--print-source', '-p'],
    "truncate-unused": ['--truncate-unused', '-t'],
}

# Strings longer than this will have extra space added between
# them in the translation files to make it easier to distinguish their
# beginnings and endings at a glance
doublespace_threshold = 80

def set_params_folders(tab: list):
    '''Initialize params["folders"] from CLI arguments.'''
    # Discarding argument 0 (tool name)
    for param in tab[1:]:
        stop_param = False
        for option in options:
            if param in options[option]:
                stop_param = True
                break
        if not stop_param:
            params["folders"].append(os.path.abspath(param))

def set_params(tab: list):
    '''Initialize params from CLI arguments.'''
    for option in options:
        for option_name in options[option]:
            if option_name in tab:
                params[option] = True
                break

def print_help(name):
    '''Prints some help message.'''
    print(f'''SYNOPSIS
    {name} [OPTIONS] [PATHS...]
DESCRIPTION
    {', '.join(options["help"])}
        prints this help message
    {', '.join(options["recursive"])}
        run on all subfolders of paths given
    {', '.join(options["mods"])}
        run on locally installed modules
    {', '.join(options["old-file"])}
        create *.old files
    {', '.join(options["sort"])}
        sort output strings alphabetically
    {', '.join(options["break-long-lines"])}
        add extra line breaks before and after long strings
    {', '.join(options["print-source"])}
	add comments denoting the source file
    {', '.join(options["verbose"])}
        add output information
    {', '.join(options["truncate-unused"])}
        delete unused strings from files
''')


def main():
    '''Main function'''
    set_params(_argv)
    set_params_folders(_argv)
    if params["help"]:
        print_help(_argv[0])
    elif params["recursive"] and params["mods"]:
        print("Option --installed-mods is incompatible with --recursive")
    else:
        # Add recursivity message
        print("Running ", end='')
        if params["recursive"]:
            print("recursively ", end='')
        # Running
        if params["mods"]:
            print(f"on all locally installed modules in {os.path.expanduser('~/.minetest/mods/')}")
            run_all_subfolders(os.path.expanduser("~/.minetest/mods"))
        elif len(params["folders"]) >= 2:
            print("on folder list:", params["folders"])
            for f in params["folders"]:
                if params["recursive"]:
                    run_all_subfolders(f)
                else:
                    update_folder(f)
        elif len(params["folders"]) == 1:
            print("on folder", params["folders"][0])
            if params["recursive"]:
                run_all_subfolders(params["folders"][0])
            else:
                update_folder(params["folders"][0])
        else:
            print("on folder", os.path.abspath("./"))
            if params["recursive"]:
                run_all_subfolders(os.path.abspath("./"))
            else:
                update_folder(os.path.abspath("./"))

#group 2 will be the string, groups 1 and 3 will be the delimiters (" or ')
#See https://stackoverflow.com/questions/46967465/regex-match-text-in-either-single-or-double-quote
pattern_lua_s = re.compile(r'[\.=^\t,{\(\s]N?S\s*\(\s*(["\'])((?:\\\1|(?:(?!\1)).)*)(\1)[\s,\)]', re.DOTALL)
pattern_lua_fs = re.compile(r'[\.=^\t,{\(\s]N?FS\s*\(\s*(["\'])((?:\\\1|(?:(?!\1)).)*)(\1)[\s,\)]', re.DOTALL)
#handles the [[ ... ]] string delimiters
pattern_lua_bracketed_s = re.compile(r'[\.=^\t,{\(\s]N?S\s*\(\s*\[\[(.*?)\]\][\s,\)]', re.DOTALL)
pattern_lua_bracketed_fs = re.compile(r'[\.=^\t,{\(\s]N?FS\s*\(\s*\[\[(.*?)\]\][\s,\)]', re.DOTALL)

# Handles "concatenation" .. " of strings"
pattern_concat = re.compile(r'["\'][\s]*\.\.[\s]*["\']', re.DOTALL)

pattern_tr = re.compile(r'(.*?[^@])=(.*)')
pattern_name = re.compile(r'^name[ ]*=[ ]*([^ \n]*)')
pattern_tr_filename = re.compile(r'\.tr$')
pattern_po_language_code = re.compile(r'(.*)\.po$')

#attempt to read the mod's name from the mod.conf file or folder name. Returns None on failure
def get_modname(folder):
    try:
        with open(os.path.join(folder, "mod.conf"), "r", encoding='utf-8') as mod_conf:
            for line in mod_conf:
                match = pattern_name.match(line)
                if match:
                    return match.group(1)
    except FileNotFoundError:
        if not os.path.isfile(os.path.join(folder, "modpack.txt")):
            folder_name = os.path.basename(folder)
            # Special case when run in Minetest's builtin directory
            if folder_name == "builtin":
                return "__builtin"
            else:
                return folder_name
        else:
            return None
    return None

#If there are already .tr files in /locale, returns a list of their names
def get_existing_tr_files(folder):
    out = []
    for root, dirs, files in os.walk(os.path.join(folder, 'locale/')):
        for name in files:
            if pattern_tr_filename.search(name):
                out.append(name)
    return out

# A series of search and replaces that massage a .po file's contents into
# a .tr file's equivalent
def process_po_file(text):
    # The first three items are for unused matches
    text = re.sub(r'#~ msgid "', "", text)
    text = re.sub(r'"\n#~ msgstr ""\n"', "=", text)
    text = re.sub(r'"\n#~ msgstr "', "=", text)
    # comment lines
    text = re.sub(r'#.*\n', "", text)
    # converting msg pairs into "=" pairs
    text = re.sub(r'msgid "', "", text)
    text = re.sub(r'"\nmsgstr ""\n"', "=", text)
    text = re.sub(r'"\nmsgstr "', "=", text)
    # various line breaks and escape codes
    text = re.sub(r'"\n"', "", text)
    text = re.sub(r'"\n', "\n", text)
    text = re.sub(r'\\"', '"', text)
    text = re.sub(r'\\n', '@n', text)
    # remove header text
    text = re.sub(r'=Project-Id-Version:.*\n', "", text)
    # remove double-spaced lines
    text = re.sub(r'\n\n', '\n', text)
    return text

# Go through existing .po files and, if a .tr file for that language
# *doesn't* exist, convert it and create it.
# The .tr file that results will subsequently be reprocessed so
# any "no longer used" strings will be preserved.
# Note that "fuzzy" tags will be lost in this process.
def process_po_files(folder, modname):
    for root, dirs, files in os.walk(os.path.join(folder, 'locale/')):
        for name in files:
            code_match = pattern_po_language_code.match(name)
            if code_match == None:
                continue
            language_code = code_match.group(1)
            tr_name = f'{modname}.{language_code}.tr'
            tr_file = os.path.join(root, tr_name)
            if os.path.exists(tr_file):
                if params["verbose"]:
                    print(f"{tr_name} already exists, ignoring {name}")
                continue
            fname = os.path.join(root, name)
            with open(fname, "r", encoding='utf-8') as po_file:
                if params["verbose"]:
                    print(f"Importing translations from {name}")
                text = process_po_file(po_file.read())
                with open(tr_file, "wt", encoding='utf-8') as tr_out:
                    tr_out.write(text)

# from https://stackoverflow.com/questions/600268/mkdir-p-functionality-in-python/600612#600612
# Creates a directory if it doesn't exist, silently does
# nothing if it already exists
def mkdir_p(path):
    try:
        os.makedirs(path)
    except OSError as exc: # Python >2.5
        if exc.errno == errno.EEXIST and os.path.isdir(path):
            pass
        else: raise

# Converts the template dictionary to a text to be written as a file
# dKeyStrings is a dictionary of localized string to source file sets
# dOld is a dictionary of existing translations and comments from
# the previous version of this text
def strings_to_text(dkeyStrings, dOld, mod_name, header_comments):
    lOut = [f"# textdomain: {mod_name}"]
    if header_comments is not None:
        lOut.append(header_comments)
    
    dGroupedBySource = {}

    for key in dkeyStrings:
        sourceList = list(dkeyStrings[key])
        if params["sort"]:
            sourceList.sort()
        sourceString = "\n".join(sourceList)
        listForSource = dGroupedBySource.get(sourceString, [])
        listForSource.append(key)
        dGroupedBySource[sourceString] = listForSource
    
    lSourceKeys = list(dGroupedBySource.keys())
    lSourceKeys.sort()
    for source in lSourceKeys:
        localizedStrings = dGroupedBySource[source]
        if params["sort"]:
            localizedStrings.sort()
        if params["print-source"]:
            if lOut[-1] != "":
                lOut.append("")
            lOut.append(source)
        for localizedString in localizedStrings:
            val = dOld.get(localizedString, {})
            translation = val.get("translation", "")
            comment = val.get("comment")
            if params["break-long-lines"] and len(localizedString) > doublespace_threshold and not lOut[-1] == "":
                lOut.append("")
            if comment != None and comment != "" and not comment.startswith("# textdomain:"):
                lOut.append(comment)
            lOut.append(f"{localizedString}={translation}")
            if params["break-long-lines"] and len(localizedString) > doublespace_threshold:
                lOut.append("")


    unusedExist = False
    if not params["truncate-unused"]:
        for key in dOld:
            if key not in dkeyStrings:
                val = dOld[key]
                translation = val.get("translation")
                comment = val.get("comment")
                # only keep an unused translation if there was translated
                # text or a comment associated with it
                if translation != None and (translation != "" or comment):
                    if not unusedExist:
                        unusedExist = True
                        lOut.append("\n\n##### not used anymore #####\n")
                    if params["break-long-lines"] and len(key) > doublespace_threshold and not lOut[-1] == "":
                        lOut.append("")
                    if comment != None:
                        lOut.append(comment)
                    lOut.append(f"{key}={translation}")
                    if params["break-long-lines"] and len(key) > doublespace_threshold:
                        lOut.append("")
    return "\n".join(lOut) + '\n'

# Writes a template.txt file
# dkeyStrings is the dictionary returned by generate_template
def write_template(templ_file, dkeyStrings, mod_name):
    # read existing template file to preserve comments
    existing_template = import_tr_file(templ_file)
    
    text = strings_to_text(dkeyStrings, existing_template[0], mod_name, existing_template[2])
    mkdir_p(os.path.dirname(templ_file))
    with open(templ_file, "wt", encoding='utf-8') as template_file:
        template_file.write(text)


# Gets all translatable strings from a lua file
def read_lua_file_strings(lua_file):
    lOut = []
    with open(lua_file, encoding='utf-8') as text_file:
        text = text_file.read()

        text = re.sub(pattern_concat, "", text)

        strings = []
        for s in pattern_lua_s.findall(text):
            strings.append(s[1])
        for s in pattern_lua_bracketed_s.findall(text):
            strings.append(s)
        for s in pattern_lua_fs.findall(text):
            strings.append(s[1])
        for s in pattern_lua_bracketed_fs.findall(text):
            strings.append(s)
                
        for s in strings:
            s = re.sub(r'"\.\.\s+"', "", s)
            s = re.sub("@[^@=0-9]", "@@", s)
            s = s.replace('\\"', '"')
            s = s.replace("\\'", "'")
            s = s.replace("\n", "@n")
            s = s.replace("\\n", "@n")
            s = s.replace("=", "@=")
            lOut.append(s)
    return lOut

# Gets strings from an existing translation file
# returns both a dictionary of translations
# and the full original source text so that the new text
# can be compared to it for changes.
# Returns also header comments in the third return value.
def import_tr_file(tr_file):
    dOut = {}
    text = None
    header_comment = None
    if os.path.exists(tr_file):
        with open(tr_file, "r", encoding='utf-8') as existing_file :
            # save the full text to allow for comparison
            # of the old version with the new output
            text = existing_file.read()
            existing_file.seek(0)
            # a running record of the current comment block
            # we're inside, to allow preceeding multi-line comments
            # to be retained for a translation line
            latest_comment_block = None
            for line in existing_file.readlines():
                line = line.rstrip('\n')
                if line.startswith("###"):
                    if header_comment is None and not latest_comment_block is None:
                        # Save header comments
                        header_comment = latest_comment_block
                        # Strip textdomain line
                        tmp_h_c = ""
                        for l in header_comment.split('\n'):
                            if not l.startswith("# textdomain:"):
                                tmp_h_c += l + '\n'
                        header_comment = tmp_h_c

                    # Reset comment block if we hit a header
                    latest_comment_block = None
                    continue
                elif line.startswith("#"):
                    # Save the comment we're inside
                    if not latest_comment_block:
                        latest_comment_block = line
                    else:
                        latest_comment_block = latest_comment_block + "\n" + line
                    continue
                match = pattern_tr.match(line)
                if match:
                    # this line is a translated line
                    outval = {}
                    outval["translation"] = match.group(2)
                    if latest_comment_block:
                        # if there was a comment, record that.
                        outval["comment"] = latest_comment_block
                    latest_comment_block = None
                    dOut[match.group(1)] = outval
    return (dOut, text, header_comment)

# Walks all lua files in the mod folder, collects translatable strings,
# and writes it to a template.txt file
# Returns a dictionary of localized strings to source file sets
# that can be used with the strings_to_text function.
def generate_template(folder, mod_name):
    dOut = {}
    for root, dirs, files in os.walk(folder):
        for name in files:
            if fnmatch.fnmatch(name, "*.lua"):
                fname = os.path.join(root, name)
                found = read_lua_file_strings(fname)
                if params["verbose"]:
                    print(f"{fname}: {str(len(found))} translatable strings")

                for s in found:
                    sources = dOut.get(s, set())
                    sources.add(f"### {os.path.basename(fname)} ###")
                    dOut[s] = sources
                    
    if len(dOut) == 0:
        return None
    templ_file = os.path.join(folder, "locale/template.txt")
    write_template(templ_file, dOut, mod_name)
    return dOut

# Updates an existing .tr file, copying the old one to a ".old" file
# if any changes have happened
# dNew is the data used to generate the template, it has all the
# currently-existing localized strings
def update_tr_file(dNew, mod_name, tr_file):
    if params["verbose"]:
        print(f"updating {tr_file}")

    tr_import = import_tr_file(tr_file)
    dOld = tr_import[0]
    textOld = tr_import[1]

    textNew = strings_to_text(dNew, dOld, mod_name, tr_import[2])

    if textOld and textOld != textNew:
        print(f"{tr_file} has changed.")
        if params["old-file"]:
            shutil.copyfile(tr_file, f"{tr_file}.old")

    with open(tr_file, "w", encoding='utf-8') as new_tr_file:
        new_tr_file.write(textNew)

# Updates translation files for the mod in the given folder
def update_mod(folder):
    modname = get_modname(folder)
    if modname is not None:
        process_po_files(folder, modname)
        print(f"Updating translations for {modname}")
        data = generate_template(folder, modname)
        if data == None:
            print(f"No translatable strings found in {modname}")
        else:
            for tr_file in get_existing_tr_files(folder):
                update_tr_file(data, modname, os.path.join(folder, "locale/", tr_file))
    else:
        print(f"\033[31mUnable to find modname in folder {folder}.\033[0m", file=_stderr)
        exit(1)

# Determines if the folder being pointed to is a mod or a mod pack
# and then runs update_mod accordingly
def update_folder(folder):
    is_modpack = os.path.exists(os.path.join(folder, "modpack.txt")) or os.path.exists(os.path.join(folder, "modpack.conf"))
    if is_modpack:
        subfolders = [f.path for f in os.scandir(folder) if f.is_dir() and not f.name.startswith('.')]
        for subfolder in subfolders:
            update_mod(subfolder)
    else:
        update_mod(folder)
    print("Done.")

def run_all_subfolders(folder):
    for modfolder in [f.path for f in os.scandir(folder) if f.is_dir() and not f.name.startswith('.')]:
        update_folder(modfolder)


main()
