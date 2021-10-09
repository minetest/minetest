#!/usr/bin/env python3

# Display versions for enabled dependencies in a build.

# usage: ./util/show_dependency_versions.py [build_dir]
# The default build dir is the current directory.

import os

REQUIRED_DEPS = ("openssl", "libsqlite3-dev", "libzstd-dev")
CMAKE_DEP_OPTION_VARS = {
    "curl" : "ENABLE_CURL",
    "lubcurses-dev" : "ENABLE_CURSES",   
    "libfreetype-dev" : "ENABLE_FREETYPE",
    "libluajit-5.1-dev" : "ENABLE_LUAJIT",
    "gettext" : "ENABLE_GETTEXT",
    "postgresql-client" : "ENABLE_POSTGRESQL",
    "libleveldb-dev" : "ENABLE_LEVELDB",
    "libhiredis-dev" : "ENABLE_REDIS",
    "libspatialindex-dev" : "ENABLE_SPATIAL",
    "libvorbis-dev" : "ENABLE_SOUND",
    "libogg-dev" : "ENABLE_SOUND",
    "libopenal-dev" : "ENABLE_SOUND",
    "prometheus" : "ENABLE_PROMETHEUS",
}

OPTION_ON_STRS = ("ON", "TRUE", "1")

# Check whether an optional depency is enabled in the build.
def is_enabled(build_dir, pkg_name):
    if not os.path.isfile("./CMakeLists.txt"):
        raise RuntimeError("Script must be executed from project root.")
    if not os.path.isfile("{}/CMakeCache.txt".format(build_dir)):
        raise RuntimeError("No CMakeCache.txt in {}.".format(build_dir))

    cache_var = CMAKE_DEP_OPTION_VARS[pkg_name]
    option_str = os.popen(
       "cmake -B {} -L | grep {}".format(build_dir, cache_var)).read()

    if not option_str:
        raise RuntimeError("Could not find CMake var {}.".format(option_name))

    option_val = option_str.strip().split('=')[1]
    return option_val.upper() in OPTION_ON_STRS

# Get installed version of a package.
# Does not handle error if package not found.
def installed_version(pkg_name):
    version_line = os.popen(
        "apt-cache policy {} | grep Installed".format(pkg_name)).read()
    return version_line.strip().split()[1]

# Return dict of {package : version} for every enabled dependency.
def get_versions(build_dir):
    pkgs = {}

    for dep in REQUIRED_DEPS:
        pkgs[dep] = installed_version(dep)

    for dep, cache_var in CMAKE_DEP_OPTION_VARS.items():
        if is_enabled(build_dir, dep):
            pkgs[dep] = installed_version(dep)

    return pkgs
            
if __name__ == "__main__":
    import sys
    # As far as I know, it is common for Minetest to build in the project root.
    build_dir = "./"
    if len(sys.argv) > 1:
        build_dir = sys.argv[1]
    
    print("\nInstalled versions of enabled_dependencies:")
    for pkg, version in get_versions(build_dir).items():
        print("{} :{}".format(pkg, version))
