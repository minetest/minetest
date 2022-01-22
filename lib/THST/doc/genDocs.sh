
doxygen html.cfg
doxygen docset.cfg
#generate docset
cd docset
make
for f in $(ls | grep -v ".docset"); do
	rm -rf $f
done

cd ..

