# Cap'n Proto files

These [Cap'n Proto](https://capnproto.org/) files define the
interface between the network interface for the
minetest client that is used by the gymnasium interface. Both the
python code and the C++ code depend on it, so it's a bit arbitrary where
in the source tree it goes, but it seems easier for CMake to depend on
something in the python directory than for setuptools to package something
outside the python directory.
