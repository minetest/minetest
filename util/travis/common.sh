#!/bin/bash -e

# Relative to git-repository root:
TRIGGER_COMPILE_PATHS="src/.*\.(c|cpp|h)|CMakeLists.txt|cmake/Modules/|util/travis/|util/buildbot/"

needs_compile() {
	git diff --name-only $TRAVIS_COMMIT_RANGE | egrep -q "^($TRIGGER_COMPILE_PATHS)"
}

