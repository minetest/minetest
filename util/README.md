# Minetest utilities

The `util` directory contains a number of helper scripts used for development
and maintenance.

## Scripts

### Translation

* `updatepo.sh`: Update Gettext files (`.po`) for engine translations
* `checktr.py`: Check mod/builtin translation files (`*.tr` and `template.txt`)
                for errors (see also `README_checktr.md`)

### Runtime tests

* `stress_mapgen.sh`: Start Minetest and start a mapgen stress-test
* `test_multiplayer.sh`: Start a Minetest server and client and test whether a connection is possible

### Maintenance

* `bump_version.sh`: Increase the Minetest version, update relevant files, make a Git commit and tag
* `gather_git_credits.sh`: Generate statistics of how active contributors have been
* `reorder_translation_commits.py`: Git commit/rebase helper for reordering translation commits from Weblate

### Other

* `fix_format.sh`: Fix coding style of source code files
* `wireshark/minetest.lua`: Wireshark plugin to dissect Minetest packets
