#!/bin/sh -e

# Use this if installed mkdocs through pipx
#PYMD_WIKILINKS="$(pipx runpip mkdocs show markdown | awk '/Location/ {print $2}')/markdown/extensions/wikilinks.py"
#PYMD_TOC="$(pipx runpip mkdocs show markdown | awk '/Location/ {print $2}')/markdown/extensions/toc.py"

# Split lua_api.md on top level headings
cat ../lua_api.md | csplit -sz -f docs/section - '/^=/-1' '{*}'

cat > mkdocs.yml << EOF
site_name: Minetest API Documentation
theme:
    name: readthedocs
    highlightjs: False
extra_css:
    - css/code_styles.css
    - css/extra.css
markdown_extensions:
    - toc:
        permalink: True
    - wikilinks
    - pymdownx.superfences
    - pymdownx.highlight:
        css_class: codehilite
plugins:
    - search:
        separator: '[\s\-\.\(]+'
nav:
- "Home": index.md
EOF

# $1: file to scrape headers from
# $2: URL path part, i.e. 'games' in 'api.minetest.net/games'
scrape_header() {
    # extract headers
    sed -n -e '/^\(=\+\|-\+\)$/{x;s!\(.*\)!    "\1": "'"$2"'",!p;d;}; x;' \
           -e '/^#\+ .\+/s!#\+ \(.\+\)!    "\1": "'"$2"'",!p' "$1" |
    # rid of unwanted parts of wikilink refs
    sed -n -e 's![`\*]\+!!g' \
           -e 's!\(\w\+\)\[[^][]*\]!\1[]!' \
           -e 's!\(\[\w\+\)[^"]*!\1!' \
           -e 's! *([^()]*)!!; p'
}

# $1 file to insert lines of text
# $2 line number to insert at
# $3 newline-separated text
insert() {
    {
    head -n $(($2-1)) "$1"
    echo "$3"
    tail -n +$2 "$1"
    } >"$1.tmp"
    mv "$1.tmp" "$1"
}

# Patch the extensions
[ -z "$PYMD_WIKILINKS" ] && PYMD_WIKILINKS="$(pip show markdown | awk '/Location/ {print $2}')/markdown/extensions/wikilinks.py"
if [ -f "$PYMD_WIKILINKS.patched" ]; then # repeat runs
    cp -f "$PYMD_WIKILINKS.patched" "$PYMD_WIKILINKS"
else # first run
    patch -N -r - "$PYMD_WIKILINKS" wikilinks.patch || true
    cp "$PYMD_WIKILINKS" "$PYMD_WIKILINKS.patched"
fi

[ -z "$PYMD_TOC" ] && PYMD_TOC="$(pip show markdown | awk '/Location/ {print $2}')/markdown/extensions/toc.py"
if [ -f "$PYMD_TOC.patched" ]; then # repeat runs
    cp -f "$PYMD_TOC.patched" "$PYMD_TOC"
else # first run
    patch -N -r - "$PYMD_TOC" toc.patch || true
    cp "$PYMD_TOC" "$PYMD_TOC.patched"
fi

mv docs/section00 docs/index.md

for f in docs/section*
do
	title=$(head -1 "$f")
	fname=$(echo "$title" | tr '[:upper:]' '[:lower:]')
	fname=$(echo "$fname" | sed "s/ /-/g; s/['\`]//g")

    inject="$inject"$'\n'"$(scrape_header "$f" "$fname")"

    fname="$fname.md"
	mv "$f" "docs/$fname"
	echo "- \"$title\": $fname" >> mkdocs.yml
done

# Inject header-to-path information
insert "$PYMD_WIKILINKS" 24 "$inject"

case "$1" in
    serve)
        mkdocs serve ;;
    *)
        mkdocs build --site-dir ../../public ;;
esac
