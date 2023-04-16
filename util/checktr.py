#!/usr/bin/env python3
# -*- coding: utf-8 -*-
#
# Script to check/builtin mod translation files (*.tr and template.txt) for Minetest.
#
# Syntax: ./checktr.py <PATH_TO_CHECK> [OPTIONS]
#
# Use option '-h' for help or read 'README_checktr.md'.
# See 'README_checktr.md' for details.

import re
import os
from sys import argv as _argv

# Available options
options = {
    "help": ['--help', '-h'],
    "list-empty": ['--list-empty', '-e'],
}
params = {
    "help": False,
    "list-empty": False,
}

# matches translation lines with non-empty source string
pattern_tr = re.compile(r'(.*?[^@])=(.*)')
# matches translation lines with empty source string
pattern_tr_empty_orig = re.compile(r'=(.*)')
# matches placeholder @1 to @9
pattern_placeholders = re.compile(r'@([1-9])')
# matches bad escape codes ('@' sign not followed by @, =, n or 1-9)
# '@' followed by a literal newline is handled in the code.
pattern_bad_escapes = re.compile(r'(@[^@=n1-9])')
# matches file names with suffix '.tr'
pattern_tr_filename = re.compile(r'\.tr$')
# matches file names with suffix '.tr' case-insensitively
pattern_tr_filename_ci = re.compile(r'\.([tT][rR])$')
# matches file names with suffix '.[LANG].tr'.
pattern_tr_lang_filename = re.compile(r'\.[^.]+\.tr$')
# same as before, but also needs a non-empty
# string before the suffix (e.g. fails for filename '.de.tr')
pattern_tr_lang_filename_prefix = re.compile(r'.+\.[^.]+\.tr$')

def get_existing_tr_files(folder):
    '''Returns list of all *.tr files in folder (case-insensively).'''
    out = []
    for root, dirs, files in os.walk(folder):
        for name in files:
            if pattern_tr_filename_ci.search(name):
                out.append(os.path.join(root, name))
    return out

def get_existing_template_files(folder):
    '''Returns list of all template.txt files in folder.'''
    out = []
    for root, dirs, files in os.walk(folder):
        for name in files:
            if name == "template.txt":
                out.append(os.path.join(root, name))
    return out

def check_tr_file(tr_file, is_template, toptions):
    '''Checks translation (*.tr) file or template.txt for errors and possible problem and prints problems on console.'''
    empty_translations = 0
    valid_translations = 0
    total_strings = 0
    warnings = 0
    errors = 0
    textdomains = 0

    # Get name of directory in which the file is stored
    # (stripping the path and filename)
    (_, tr_directory) = os.path.split(os.path.dirname(tr_file))

    # Check if the file is stored in a 'locale' directory.
    # Note: Does NOT check if this directory in turn is in a mod or builtin.
    if tr_directory != "locale":
        if is_template:
            # for template.txt, this is only a warning (because the file name
            # might mean something different)
            print("Warning ("+tr_file+"): Template file is not in a 'locale' directory")
            warnings += 1
        else:
            print("Error ("+tr_file+"): Translation file is not in a 'locale' directory")
            errors += 1

    # Check syntax of .tr file name
    if not is_template:
        tr_file_basename = os.path.basename(tr_file)

        ci_match = pattern_tr_filename_ci.findall(tr_file_basename)
        # Check for uppercase character in suffix (e.g. '.TR')
        if ci_match and ci_match[0][0].isupper() and ci_match[0][1].isupper():
            errors += 1
            print("Error ("+tr_file+"): File name suffix uses uppercase")
        # Check for this file name syntax:
        #    [SOMETHING].[LANG].tr
        # where [SOMETHING] and [LANG] are strings of length >= 1
        # that don't contain a period character
        elif not pattern_tr_lang_filename.search(tr_file_basename):
            errors += 1
            print("Error ("+tr_file+"): Missing language code in file name")
        elif not pattern_tr_lang_filename_prefix.search(tr_file_basename):
            warnings += 1
            print("Warning ("+tr_file+"): Empty prefix in file name")

    if os.path.exists(tr_file):
        try:
            existing_file = open(tr_file, "r", encoding='utf-8')
            existing_file.seek(0)

            # line counter
            lineno = 0

            # previous line, for newline escaping
            prevline = ""

            for line in existing_file.readlines():
                line = prevline + line
                prevline = ""

                # remove trailing newline
                line = line.rstrip("\n")

                # Replace escaped '@' sign with ASCII SUB control char
                # so the double '@' is out of the way for our
                # regexes.
                line.replace("@@", "\x1A")

                # count line number (for error+warning messages)
                lineno = lineno + 1

                lineinfo = tr_file + ":" + str(lineno)

                # Textdomain line
                if line.startswith("# textdomain:"):
                    textdomains += 1
                    textdomain = line.removeprefix("# textdomain:").strip()
                    if textdomain == "":
                        print("Error ("+lineinfo+"): Empty textdomain")
                        errors += 1
                    continue
                # Bad textdomain line
                elif line.startswith("#textdomain"):
                    print("Warning ("+lineinfo+"): Missing space between '#' and 'textdomain'")
                    warnings += 1
                    continue
                # Regular comment
                elif line.startswith("#"):
                    continue
                # Empty line
                elif line == "":
                    continue
                # Escape code: @ followed by literal newline.
                # (unless it's the end of file, which triggers an error)
                elif line.endswith("@"):
                    # Store this line for the next iteration
                    # Add a 'n' so the escape becomes '@n'
                    prevline = line + "n"
                    continue
                # Other line (translation or invalid)
                else:
                    match1 = pattern_tr.match(line)
                    match2 = pattern_tr_empty_orig.match(line)
                    orig = None
                    translation = None
                    if match1:
                        orig = match1[1]
                        translation = match1[2]
                        match = match2
                    elif match2:
                        orig = ""
                        translation = match2[1]
                        match = match2
                    if orig != None and translation != None :
                        total_strings = total_strings + 1
                        match_esc = pattern_bad_escapes.match(orig)
                        failed = False

                        if orig == "":
                            print("Warning ("+lineinfo+"): Empty source string")
                            warnings += 1

                        # Check for invalid escape codes (invalid character after '@')
                        if match_esc:
                            print("Error ("+lineinfo+"): Bad escape code in source string: '"+match_esc[1]+"'")
                            errors += 1
                            failed = True
                        match_esc = pattern_bad_escapes.match(translation)
                        if match_esc:
                            print("Error ("+lineinfo+"): Bad escape code in translation: '"+match_esc[1]+"'")
                            errors += 1
                            failed = True

                        # template.txt must only have empty translations
                        if is_template and translation != "":
                            if translation.isspace():
                                print("Error ("+lineinfo+"): Whitespace-only translation in template file (translation must be empty)")
                            else:
                                print("Error ("+lineinfo+"): Non-empty translation in template file")
                            errors += 1
                            failed = True

                        placeholders = pattern_placeholders.findall(orig)
                        t_placeholders = pattern_placeholders.findall(translation)
                        # check placeholders ('@1', '@2', etc.)
                        if placeholders:
                            # Check for the correct order of placeholders in the source string
                            # (must start at @1 and then increase with every placeholder)
                            expected_num = 1 # next expected placeholder number
                            syms = {} # map of found placeholders
                            syms_num = 0 # number of found placeholders
                            bad_src_placeholders = False # True if placeholders in source string are bad
                            for i in placeholders:
                                if i in syms and syms[i] == True:
                                    print("Error ("+lineinfo+"): Placeholder '@"+str(i)+"' in source string was repeated")
                                    errors += 1
                                    failed = True
                                    bad_src_placeholders = True
                                else:
                                    syms_num += 1
                                syms[i] = True
                                if not failed and i != str(expected_num):
                                    print("Error ("+lineinfo+"): Placeholder '@"+str(i)+"' in source string is not in the correct ascending order")
                                    errors += 1
                                    failed = True
                                    bad_src_placeholders = True
                                expected_num += 1

                            # Check for missing placeholders in translation.
                            # All placeholders of the source string must appear at least once in the translation.
                            # Also checks for placeholders in translation that are *not* in the source string.
                            # Do NOT check the translation if the source string is broken.
                            if translation != "" and not bad_src_placeholders:
                                extra_sym = None
                                if t_placeholders:
                                    for i in t_placeholders:
                                        if i in syms:
                                            if syms[i] == True:
                                               syms[i] = False
                                               syms_num -= 1
                                        else:
                                            extra_sym = i
                                    if syms_num > 0:
                                        # List all missing placeholders
                                        missing_str = ""
                                        for i in syms:
                                            if syms[i] == True:
                                                missing_str = missing_str + "'@"+str(i)+"' "
                                        print("Error ("+lineinfo+"): Missing placeholder(s) in translation: "+missing_str)
                                        errors += 1
                                        failed = True

                                    # Excess placeholders in translation
                                    if extra_sym != None:
                                        print("Error ("+lineinfo+"): Placeholder '@"+extra_sym+"' in translation doesn't exist in source string")
                                        errors += 1
                                        failed = True
                                else:
                                    # List all missing placeholders
                                    missing_str = ""
                                    for i in syms:
                                        missing_str = missing_str + "'@"+str(i)+"' "
                                    print("Error ("+lineinfo+"): Missing placeholder(s) in translation: "+missing_str)
                                    errors += 1
                                    failed = True

                        elif t_placeholders:
                            print("Error ("+lineinfo+"): Placeholder(s) found in translation but source string has no placeholder")
                            errors += 1
                            failed = True

                        # Empty translation for non-empty source string
                        if (not is_template) and translation == "" and orig != "":
                            empty_translations = empty_translations + 1
                            failed = True

                        if failed:
                            continue
                        # If we reach this part, all checks have passed and we count this line as valid
                        valid_translations = valid_translations + 1

                    else:
                        # Line was either comment, empty or a translation line: This is a syntax error
                        if line.isspace():
                            # more helpful error message for whitespace-only line
                            print("Error ("+lineinfo+"): Line contains only whitespace (must be empty)")
                        else:
                            print("Error ("+lineinfo+"): Invalid line type (neither empty, comment, textdomain or translation)")
                        errors = errors + 1

            if prevline != "":
                print("Error ("+lineinfo+"): Unescaped '@' at end of file")
                errors += 1

            if textdomains == 0:
                print("Warning ("+tr_file+"): No textdomain in file")
                warnings += 1

            if toptions["list_empty"] and (not is_template):
                if empty_translations > 0:
                    print("Note ("+tr_file+"): "+str(empty_translations)+" empty translation(s)")
        except ValueError:
            print("Error ("+tr_file+"): UTF-8 encoding error in file")
            errors += 1
        finally:
            existing_file.close()
    else:
        print("Error ("+tr_file+"): File not found")
        errors += 1

    return (errors, warnings, empty_translations, total_strings)

def run_tests(path, toptions):
    '''Test all locale files in path and subdirectories'''
    files = 0
    filefails = 0
    filewarns = 0
    empties = 0
    tr_files = get_existing_tr_files(path)
    for file in tr_files:
        files += 1
        (errors, warnings, empty_translations, total_strings) = check_tr_file(file, False, toptions)
        if errors > 0:
            filefails += 1
        if warnings > 0:
            filewarns += 1
        if empty_translations > 0:
            empties += 1

    template_files = get_existing_template_files(path)
    for file in template_files:
        files += 1
        (errors, warnings, empty_translations, total_strings) = check_tr_file(file, True, toptions)
        if errors > 0:
            filefails += 1
        if warnings > 0:
            filewarns += 1

    if files == 0:
        print("No *.tr or template.txt files found in "+path)
        return True

    print("------------")
    print("TEST RESULT:")
    print("------------")
    if filefails == 0:
        print(str(files) + " file(s) checked. Test PASSED!")
        if filewarns > 0:
            print(str(filewarns) + " file(s) with warnings.")
        if toptions["list_empty"] or empties > 0:
            print(str(empties) + " file(s) with empty translations.")
        return True
    else:
        print(str(files) + " file(s) checked. Test FAILED!")
        print(str(filefails) + " file(s) with error(s).")
        if filewarns > 0:
            print(str(filewarns) + " file(s) with warnings.")
        if toptions["list_empty"] or empties > 0:
            print(str(empties) + " file(s) with empty translations.")
        return False


def set_params(tab: list):
    '''Initialize params from CLI arguments.'''
    for option in options:
        for option_name in options[option]:
            if option_name in tab:
                params[option] = True
                break

def print_help():
    '''Print help about this program to console.'''
    print(
"""This script checks for errors in Minetest mod translation files
(*.tr and template.txt) and prints any errors on the console.

Syntax:
    """+_argv[0]+""" <PATH> [OPTIONS]

<PATH> is the path to check (default: working directory).
OPTIONS is a list of optional options. These options are supported:

    -h, --help: Print help and exits program
    -e, --list-empty: List files with empty translations""")

def main():
    '''Starting point of the program.'''
    # Scan parameters
    set_params(_argv)
    toptions = {
        "list_empty": False,
    }
    if params["help"]:
        print_help()
        return
    if params["list-empty"]:
        toptions["list_empty"] = True
    # Run tests
    path = "."
    if len(_argv) > 1:
        path = _argv[1]
    else:
        print("No path specified.")
        print("Use '"+_argv[0]+" --help' for help.")
        return
    run_tests(path, toptions)

main()

