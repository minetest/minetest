#!/bin/bash


find util/fstest/tests -type f -name "*.expected.png" -delete
find util/fstest/tests  -name "*.result.png" -exec bash -c 'cp $0 ${0/.result./.expected.}' {} \;
