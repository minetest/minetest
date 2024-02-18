# Miscellaneous

## Profiling Minetest on Linux

We will be using a tool called "perf", which you can get by installing `perf` or `linux-perf` or `linux-tools-common`.

For best results build Minetest and Irrlicht with debug symbols
(`-DCMAKE_BUILD_TYPE=RelWithDebInfo` or `-DCMAKE_BUILD_TYPE=Debug`).

Run the client (or server) like this and do whatever you wanted to test:
```bash
perf record -z --call-graph dwarf -- ./bin/minetest
```

This will leave a file called "perf.data".

You can open this file with perf built-in tools but much more interesting
is the visualization using a GUI tool: **[Hotspot](https://github.com/KDAB/hotspot)**.
It will give you flamegraphs, per-thread, per-function views and much more.
