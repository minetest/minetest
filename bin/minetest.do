#!/bin/sh
set -eu

SRC_DIR="${PWD}"/../src
OBJECTS=$(
 cd ${SRC_DIR}
 for file in $(find "${SRC_DIR}" -name '*.cpp' -o -name '*.c'); do
  FILENAME=$(realpath "${file%.*}.o")
  case "${FILENAME}" in
   *"/porting_android.o"|*"/settings_translation_file.o")
    continue
   ;;
   *)
    printf '%s\n' "${FILENAME}"
   ;;
  esac
 done
)

redo-ifchange ${OBJECTS}

deps=minetest.deps
deps_ne=minetest.deps_ne

redo-ifchange "${SRC_DIR}"/CXXFLAGS
CXXFLAGS="$(cat "${SRC_DIR}"/CXXFLAGS)"

redo-ifchange "${SRC_DIR}"/LIBRARIES
LIBRARIES=$(cat "${SRC_DIR}"/LIBRARIES)

IRRLICHTMT=$(realpath "${SRC_DIR}"/../lib/irrlichtmt/lib/Linux/libIrrlichtMt.a)
redo-ifchange "${IRRLICHTMT}"

LIBFLAGS="$(printf -- '-l%s ' ${LIBRARIES})"

if command -v strace >/dev/null; then
 # Record non-existence header dependencies.
 # If headers C++ does not find are produced
 # in the future, the target is built again.
 : >"${deps_ne}.in"  # Ensure a file exists.
 strace -e stat,stat64,fstat,fstat64,lstat,lstat64 -o "${deps_ne}.in" -f \
  c++ ${CXXFLAGS} -o "${3}" ${OBJECTS} ${IRRLICHTMT} ${LIBFLAGS}
 grep '1 ENOENT' <"${deps_ne}.in"\
  |grep '".*"'\
  |cut -d'"' -f2\
  |sort \
  |uniq \
  >"${deps_ne}"
 grep --invert-match '1 ENOENT' <"${deps_ne}.in"\
  |grep '".*"'\
  |cut -d'"' -f2\
  |sort \
  |uniq \
  >"${deps}"

 while read -r dependency; do
  test -f "${dependency}" && redo-ifchange  "${dependency}"
 done <${deps}
 while read -r dependency_ne; do
  test -f "${dependency_ne}" || redo-ifcreate  "${dependency_ne}"
 done <${deps_ne}

 unlink "${deps_ne}.in"
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
 c++ ${CXXFLAGS} -o "${3}" ${OBJECTS} ${IRRLICHTMT} ${LIBFLAGS}
fi
