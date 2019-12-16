#!/bin/bash

find util/fstest/tests -type f -name "*.result.png" -delete
rm util/fstest/reports/*

OPT="-screen 0 1980x1080x24 -dpi 96"
exec xvfb-run -d -s "$OPT" python util/fstest/fstest.py
