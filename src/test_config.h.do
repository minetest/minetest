#!/bin/sh
set -eu
cat <<EOF
// Filled in by the build system

#pragma once

#define TEST_WORLDDIR "${PWD}/unittest/test_world"
#define TEST_SUBGAME_PATH "${PWD}/unittest/../../games/devtest"
#define TEST_MOD_PATH "${PWD}/unittest/test_mod"
EOF
