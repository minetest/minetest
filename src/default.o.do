#!/bin/sh
set -eu

FILENAME=$(realpath "${1}")
BASE_DIR=${FILENAME%/src/*}
SRC_DIR="${BASE_DIR}/src"

compiler_deps="${FILENAME%.o}".compiler_deps
strace="${FILENAME%.o}".strace
strace_deps="${FILENAME%.o}".strace_deps
strace_deps_ne="${FILENAME%.o}".strace_deps_ne

# FIXME: This code generates 3 unecessary dependency relations on the first build.
# This means a single file might be rebuild unnecessarily on a later build, once.
GENERATED_HEADERS="cmake_config.h cmake_config_githash.h test_config.h"
for GENERATED_HEADER in ${GENERATED_HEADERS}; do
 HEADER_PATH="${SRC_DIR}/${GENERATED_HEADER}"
 test -e "${HEADER_PATH}" || redo-ifchange "${HEADER_PATH}"
done

redo-ifchange "${SRC_DIR}/INCLUDE_DIRS" "${SRC_DIR}/CXXFLAGS"
INCLUDE_DIRS="${SRC_DIR} ${SRC_DIR}/script $(realpath ${SRC_DIR}/../lib/irrlichtmt/include) $(realpath ${SRC_DIR}/../lib/catch2) $(cat ${SRC_DIR}/INCLUDE_DIRS )"
CXXFLAGS="$(printf -- '-I%s ' ${INCLUDE_DIRS}) $(cat ${SRC_DIR}/CXXFLAGS)"

case ${FILENAME} in
 *benchmark/benchmark.o) ;;
 *)
  # Only use precompiled header if it exists, i.e. was built manually.
  if test -e "${SRC_DIR}/precompile.h.gch"; then
   CXXFLAGS="${CXXFLAGS} -include precompile.h"
  else
   redo-ifcreate "${SRC_DIR}/precompile.h.gch"
  fi
 ;;
esac

SRC=$(
 find  "$(dirname ${FILENAME})" \
  -maxdepth 1 \
  -name "$(basename "${FILENAME%.o}.cpp")" \
  -o \
  -name "$(basename "${FILENAME%.o}.c")"
 )

if command -v strace >/dev/null; then
 # Record non-existence header dependencies.
 # If headers C++ does not find are produced
 # in the future, the target is built again.
 : >"${strace}"  # Ensure a file exists.
 strace -e stat,stat64,fstat,fstat64,lstat,lstat64 -o "${strace}" -f \
  c++ ${CXXFLAGS} -o "${3}"  -c "${SRC}"
 grep '1 ENOENT' <"${strace}"\
  |grep '".*"'\
  |cut -d'"' -f2\
  |sort \
  |uniq \
  >"${strace_deps_ne}"

 while read -r dependency_ne; do
  test -f "${dependency_ne}" || redo-ifcreate  "${dependency_ne}"
 done <"${strace_deps_ne}"
 unlink "${strace_deps_ne}"

 grep --invert-match '1 ENOENT' <"${strace}"\
  |grep '".*"'\
  |cut -d'"' -f2\
  |sort \
  |uniq \
  >"${strace_deps}"
 while read -r dependency; do
  test -f "${dependency}" && redo-ifchange  "${dependency}"
 done <"${strace_deps}"
 unlink "${strace_deps}"

 unlink "${strace}"
else
 # Record non-existence strace dependency.
 # When strace is installed in the future,
 # the target is built again, with missing
 # headers recorded as non-existence deps.
 (
  IFS=:
  for folder in ${PATH}; do
   echo "$folder/strace"
  done | xargs redo-ifcreate
 )
 c++ ${CXXFLAGS} -o "${3}" -c "${SRC}"  -MD -MF "${compiler_deps}"
 read DEPS <"${compiler_deps}"
 : ${DEPS#*:}
 redo-ifchange ${DEPS#*:}
 unlink "${compiler_deps}"
fi
