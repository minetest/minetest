#!/bin/sh
set -eu
cat <<EOF
// Filled in by the build system
// Separated from cmake_config.h to avoid excessive rebuilds on every commit

#pragma once

#define VERSION_GITHASH "5.6.0-dev-$(git rev-parse --short HEAD)"
EOF
