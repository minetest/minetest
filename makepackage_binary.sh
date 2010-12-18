#!/bin/sh

PACKAGEDIR=../minetest-packages
PACKAGENAME=minetest-c55-binary-`date +%y%m%d%H%M%S`
PACKAGEPATH=$PACKAGEDIR/$PACKAGENAME

mkdir -p $PACKAGEPATH
mkdir -p $PACKAGEPATH/bin
mkdir -p $PACKAGEPATH/data
mkdir -p $PACKAGEPATH/doc

cp minetest.conf.example $PACKAGEPATH/

cp bin/minetest.exe $PACKAGEPATH/bin/
cp bin/Irrlicht.dll $PACKAGEPATH/bin/
cp bin/zlibwapi.dll $PACKAGEPATH/bin/
#cp bin/test $PACKAGEPATH/bin/
cp bin/fasttest $PACKAGEPATH/bin/
cp ../irrlicht/irrlicht-1.7.1/lib/Linux/libIrrlicht.a $PACKAGEPATH/bin/
cp ../jthread/jthread-1.2.1/src/.libs/libjthread-1.2.1.so $PACKAGEPATH/bin/

cp -r data/fontlucida.png $PACKAGEPATH/data/
cp -r data/player.png $PACKAGEPATH/data/
cp -r data/player_back.png $PACKAGEPATH/data/
cp -r data/stone.png $PACKAGEPATH/data/
cp -r data/grass.png $PACKAGEPATH/data/
cp -r data/grass_footsteps.png $PACKAGEPATH/data/
cp -r data/water.png $PACKAGEPATH/data/
cp -r data/tree.png $PACKAGEPATH/data/
cp -r data/leaves.png $PACKAGEPATH/data/
cp -r data/mese.png $PACKAGEPATH/data/
cp -r data/cloud.png $PACKAGEPATH/data/
cp -r data/sign.png $PACKAGEPATH/data/
cp -r data/sign_back.png $PACKAGEPATH/data/
cp -r data/rat.png $PACKAGEPATH/data/
cp -r data/mud.png $PACKAGEPATH/data/
cp -r data/torch.png $PACKAGEPATH/data/
cp -r data/torch_on_floor.png $PACKAGEPATH/data/
cp -r data/torch_on_ceiling.png $PACKAGEPATH/data/
cp -r data/skybox1.png $PACKAGEPATH/data/
cp -r data/skybox2.png $PACKAGEPATH/data/
cp -r data/skybox3.png $PACKAGEPATH/data/
cp -r data/tree_top.png $PACKAGEPATH/data/
cp -r data/mud_with_grass.png $PACKAGEPATH/data/

cp -r data/pauseMenu.gui $PACKAGEPATH/data/

cp -r doc/README.txt $PACKAGEPATH/doc/README.txt

cd $PACKAGEDIR
rm $PACKAGENAME.zip
zip -r $PACKAGENAME.zip $PACKAGENAME

