#!/bin/bash
dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
gameid=devtest
minetest=$dir/../bin/minetest
testspath=$dir/../tests
conf_client1=$testspath/client1.conf
conf_server=$testspath/server.conf
worldpath=$testspath/world

waitfor () {
	n=30
	while [ $n -gt 0 ]; do
		[ -f "$1" ] && return 0
		sleep 0.5
		((n-=1))
	done
	echo "Waiting for ${1##*/} timed out"
	pkill -P $$
	exit 1
}

gdbrun () {
	gdb -q -ex 'set confirm off' -ex 'r' -ex 'bt' -ex 'quit' --args "$@"
}

[ -e $minetest ] || { echo "executable $minetest missing"; exit 1; }

rm -rf $worldpath
mkdir -p $worldpath/worldmods/test

printf '%s\n' >$testspath/client1.conf \
	video_driver=null name=client1 viewing_range=10 \
	enable_{sound,minimap,shaders}=false

printf '%s\n' >$testspath/server.conf \
	max_block_send_distance=1

cat >$worldpath/worldmods/test/init.lua <<"LUA"
core.after(0, function()
	io.close(io.open(core.get_worldpath() .. "/startup", "w"))
end)
core.register_on_joinplayer(function(player)
	io.close(io.open(core.get_worldpath() .. "/player_joined", "w"))
	core.request_shutdown("", false, 2)
end)
LUA

echo "Starting server"
gdbrun $minetest --server --config $conf_server --world $worldpath --gameid $gameid 2>&1 | sed -u 's/^/(server) /' &
waitfor $worldpath/startup

echo "Starting client"
gdbrun $minetest --config $conf_client1 --go --address 127.0.0.1 2>&1 | sed -u 's/^/(client) /' &
waitfor $worldpath/player_joined

echo "Waiting for client and server to exit"
wait

echo "Success"
exit 0
