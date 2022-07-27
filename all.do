#!/bin/sh
set -eu

# FIXME: Horrible hack, should be its own build rule.
# Asking CMake to build IrrlichtMt should be okay rn.
cat <<EOF >lib/irrlichtmt/lib/Linux/libIrrlichtMt.a.do
#!/bin/sh
set -eu

# Always rebuild IrrlichtMT, unless its revision changed.
redo-always
git rev-parse HEAD |redo-stamp

(
 cd ../..
 cmake . -DBUILD_SHARED_LIBS=OFF
 make
) >&2
mv "\${1}" "\${3}"
EOF

redo-ifchange bin/minetest
