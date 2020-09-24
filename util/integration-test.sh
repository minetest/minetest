#!/bin/sh

# configuration file location and volume name
CFG=/tmp/minetest.conf
MINETEST_VOLUME=minetest

# set proper permissions in the volume
docker run --rm -i -v ${MINETEST_VOLUME}:/minetest alpine /bin/sh -c "chown -R 30000.30000 /minetest"

# generate minetest.conf
cat <<EOF > ${CFG}
 devtest_unittests_autostart = true
 enable_integration_test = true
 default_game = devtest
EOF

# start the container
docker run --rm -i \
	-v ${CFG}:/etc/minetest/minetest.conf \
	-v $(pwd)/games:/usr/local/share/minetest/games \
	-v ${MINETEST_VOLUME}:/var/lib/minetest/.minetest \
	minetest

# check if the marker file for success exists
docker run --rm -i -v ${MINETEST_VOLUME}:/minetest alpine /bin/sh -c "test -f /minetest/worlds/world/test_passed" && exit 0 || exit 1
