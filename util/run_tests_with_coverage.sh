#!/bin/sh
(cmake . -DCMAKE_BUILD_TYPE=Debug && make -j3) || exit 1
./bin/minetest --run-unittests
rm coverage -r
mkdir coverage
lcov --directory src/CMakeFiles/minetest.dir/ --base-directory src/ --no-external --capture --output-file coverage/lcov.info
lcov --remove coverage/lcov.info "*src/unittest*" -o coverage/lcov.info
genhtml --output-directory coverage coverage/lcov.info
zip -r coverage.zip coverage