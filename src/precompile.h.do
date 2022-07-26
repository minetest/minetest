#!/bin/sh
set -eu

HEADERS=$(
 find . -name '*.h'\
  |cut -c3- \
  |grep -v 'precompile.h\|porting_android.h\|database/database-postgresql.h\|util/md32_common.h\|gettext.h\|gui/guiKeyChangeMenu.h\|cmake_config\|test_config'
)

for HEADER in ${HEADERS}; do
 printf '#include "%s"\n' "${HEADER}"
done

redo-ifchange ${HEADERS}
