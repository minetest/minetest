#!/bin/bash

find util/fstest/tests -type f -name "*.result.png" -delete
exec python util/fstest/fstest.py
