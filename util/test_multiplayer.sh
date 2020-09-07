#!/bin/bash
dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
gameid=devtest
minetest=$dir/../bin/minetest
testspath=$dir/../tests
worldpath=$testspath/testworld_$gameid
configpath=$testspath/configs
logpath=$testspath/log
conf_server=$configpath/minetest.conf.multi.server
conf_client1=$configpath/minetest.conf.multi.client1
conf_client2=$configpath/minetest.conf.multi.client2
log_server=$logpath/server.log
log_client1=$logpath/client1.log
log_client2=$logpath/client2.log

mkdir -p $worldpath
mkdir -p $configpath
mkdir -p $logpath

echo -ne 'client1::shout,interact,settime,teleport,give
client2::shout,interact,settime,teleport,give
' > $worldpath/auth.txt

echo -ne '' > $conf_server

echo -ne '# client 1 config
screenW=500
screenH=380
name=client1
viewing_range_nodes_min=10
' > $conf_client1

echo -ne '# client 2 config
screenW=500
screenH=380
name=client2
viewing_range_nodes_min=10
' > $conf_client2

echo $(sleep 1; $minetest --disable-unittests --logfile $log_client1 --config $conf_client1 --go --address localhost) &
echo $(sleep 2; $minetest --disable-unittests --logfile $log_client2 --config $conf_client2 --go --address localhost) &
$minetest --disable-unittests --server --logfile $log_server --config $conf_server --world $worldpath --gameid $gameid

