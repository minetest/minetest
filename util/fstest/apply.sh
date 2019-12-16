#!/bin/bash

find util/fstest/reports  -name "*.result.png" -exec bash -c 'cp $0 ${0/.result./.expected.}' {} \;
cp util/fstest/reports/*.expected.png util/fstest/tests/
rm util/fstest/reports/*.expected.png
