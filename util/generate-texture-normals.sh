#!/bin/bash

# This script generates normalmaps using The GIMP to do the heavy lifting.
# give any unrecognized switch (say, -h) for usage info.

rm /tmp/normals_filelist.txt

numprocs=6

skiptools=false
skipinventory=false
invresolution=64
dryrun=false
pattern="*.png *.jpg"

filter=0
scale=8
wrap=0
heightsource=0
conversion=0
invertx=0
inverty=0

while test -n "$1"; do
	case "$1" in
		--scale|-s)
			if [ -z "$2" ] ; then echo "Missing scale parameter"; exit 1; fi
			scale=$2
			shift
			shift
			;;
		--pattern|-p)
			if [ -z "$2" ] ; then echo "Missing pattern parameter"; exit 1; fi
			pattern=$2
			shift
			shift
			;;
		--skiptools|-t)
			skiptools=true
			shift
			;;
		--skipinventory|-i)
			if [[ $2 =~ ^[0-9]+$ ]]; then
				invresolution=$2
				shift
			fi
			skipinventory=true
			shift
			;;
		--filter|-f)
			if [ -z "$2" ] ; then echo "Missing filter parameter"; exit 1; fi

			case "$2" in
				sobel3|1)
					filter=1
					;;
				sobel5|2)
					filter=2
					;;
				prewitt3|3)
					filter=3
					;;
				prewitt5|4)
					filter=4
					;;
				3x3|5)
					filter=5
					;;
				5x5|6)
					filter=6
					;;
				7x7|7)
					filter=7
					;;
				9x9|8)
					filter=8
					;;
				*)
					filter=0
					;;
			esac

			shift
			shift
			;;
		--heightalpha|-a)
			heightsource=1
			shift
			;;
		--conversion|-c)
				if [ -z "$2" ] ; then echo "Missing conversion parameter"; exit 1; fi

				case "$2" in
					biased|1)
						conversion=1
						;;
					red|2)
						conversion=2
						;;
					green|3)
						conversion=3
						;;
					blue|4)
						conversion=4
						;;
					maxrgb|5)
						conversion=5
						;;
					minrgb|6)
						conversion=6
						;;
					colorspace|7)
						conversion=7
						;;
					normalize-only|8)
						conversion=8
						;;
					heightmap|9)
						conversion=9
						;;
					*)
						conversion=0
						;;
			esac

			shift
			shift
			;;
		--wrap|-w)
			wrap=1
			shift
			;;
		--invertx|-x)
			invertx=1
			shift
			;;
		--inverty|-y)
			inverty=1
			shift
			;;
		--dryrun|-d)
			dryrun=true
			shift
			;;
		*)
			echo -e "\nUsage:\n"
			echo "`basename $0` [--scale|-s <value>] [--filter|-f <string>]"
			echo " [--wrap|-w] [--heightalpha|-a] [--invertx|-x] [--inverty|-y]"
			echo " [--conversion|-c <string>] [--skiptools|-t] [--skipinventory|-i [<value>]]"
			echo " [--dryrun|-d] [--pattern|-p <pattern>]"
			echo -e "\nDefaults to a scale of 8, checking all files in the current directory, and not"
			echo "skipping apparent tools or inventory images.  Filter, if specified, may be one"
			echo "of: sobel3, sobel5, prewitt3, prewitt5, 3x3, 5x5, 7x7, or 9x9, or a value 1"
			echo "through 8 (1=sobel3, 2=sobel5, etc.). Defaults to 0 (four-sample).  The height"
			echo "source is taken from the image's alpha channel if heightalpha is specified.\n"
			echo ""
			echo "If inventory skip is specified, an optional resolution may also be included"
			echo "(default is 64).  Conversion can be one of: biased, red, green, blue, maxrgb,"
			echo "minrgb, colorspace, normalize-only, heightmap or a value from 1 to 9"
			echo "corresponding respectively to those keywords.  Defaults to 0 (simple"
			echo "normalize) if not specified.  Wrap, if specified, enables wrapping of the"
			echo "normalmap around the edges of the texture (defaults to no).  Invert X/Y"
			echo "reverses the calculated gradients  for the X and/or Y dimensions represented"
			echo "by the normalmap (both default to non-inverted)."
			echo ""
			echo "The pattern, can be an escaped pattern string such as \*apple\* or"
			echo "default_\*.png or similar (defaults to all PNG and JPG images in the current"
			echo "directory that do not contain \"_normal\" or \"_specular\" in their filenames)."
			echo ""
			echo "If set for dry-run, the actions this script will take will be printed, but no"
			echo "images will be generated.  Passing an invalid value to a switch will generally"
			echo "cause that switch to revert to its default value."
			echo ""
			exit 1
			;;
	esac
done

echo -e "\nProcessing files based on pattern \"$pattern\" ..."

normalMap()
{
	out=`echo "$1" | sed 's/.png/_normal.png/' | sed 's/.jpg/_normal.png/'`

	echo "Launched process to generate normalmap: \"$1\" --> \"$out\"" >&2

	gimp -i -b "
		(define
			(normalMap-fbx-conversion fileName newFileName filter nscale wrap heightsource conversion invertx inverty)
			(let*
				(
					(image (car (gimp-file-load RUN-NONINTERACTIVE fileName fileName)))
					(drawable (car (gimp-image-get-active-layer image)))
					(drawable (car (gimp-image-flatten image)))
				)
				(if (> (car (gimp-drawable-type drawable)) 1)
					(gimp-convert-rgb image) ()
				)

				(plug-in-normalmap 
					RUN-NONINTERACTIVE
					image
					drawable
					filter
					0.0
					nscale
					wrap
					heightsource
					0
					conversion
					0
					invertx
					inverty
					0
					0.0
					drawable)
				(gimp-file-save RUN-NONINTERACTIVE image drawable newFileName newFileName)
				(gimp-image-delete image)
			)
		)
		(normalMap-fbx-conversion \"$1\" \"$out\" $2 $3 $4 $5 $6 $7 $8)" -b '(gimp-quit 0)'
}

export -f normalMap

for file in `ls $pattern |grep -v "_normal.png"|grep -v "_specular"` ; do

	invtest=`file "$file" |grep "$invresolution x $invresolution"`
	if $skipinventory && [ -n "$invtest" ] ; then
		echo "Skipped presumed "$invresolution"px inventory image: $file" >&2
		continue
	fi

	tooltest=`echo "$file" \
		| grep -v "_tool" \
		| grep -v "_shovel" \
		| grep -v "_pick" \
		| grep -v "_axe" \
		| grep -v "_sword" \
		| grep -v "_hoe" \
		| grep -v "bucket_"`

	if $skiptools && [ -z "$tooltest" ] ; then
		echo "Skipped presumed tool image: $file" >&2
		continue
	fi

	if $dryrun ; then
		echo "Would have generated a normalmap for $file" >&2
		continue
	else
		echo \"$file\" $filter $scale $wrap $heightsource $conversion $invertx $inverty
	fi
done | xargs -P $numprocs -n 8 -I{} bash -c normalMap\ \{\}\ \{\}\ \{\}\ \{\}\ \{\}\ \{\}\ \{\}\ \{\}

