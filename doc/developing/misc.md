# Miscellaneous

## Profiling Minetest on Linux

We will be using a tool called "perf", which you can get by installing `perf` or `linux-perf` or `linux-tools-common`.

To get usable results you need to build Minetest with debug symbols
(`-DCMAKE_BUILD_TYPE=RelWithDebInfo` or `-DCMAKE_BUILD_TYPE=Debug`).

Run the client (or server) like this and do whatever you wanted to test:
```bash
perf record -z --call-graph dwarf -- ./bin/minetest
```

This will leave a file called "perf.data".

You can open this file with perf built-in tools but much more interesting
is the visualization using a GUI tool: **[Hotspot](https://github.com/KDAB/hotspot)**.
It will give you flamegraphs, per-thread, per-function views and much more.

### Remote Profiling

Attach perf to your running server, press *^C* to stop:
```bash
perf record -z --call-graph dwarf -F 400 -p "$(pidof minetestserver)"
```

Collect a copy of the required libraries/executables:
```bash
perf buildid-list | grep -Eo '/[^ ]+(minetestserver|\.so)[^ ]*$' | \
	tar -cvahf debug.tgz --files-from=- --ignore-failed-read
```

Give both files to the developer and also provide:
* Linux distribution and version
* commit the source was built from and/or modified source code (if applicable)

Hotspot will resolve symbols correctly when pointing the sysroot option at the collected libs.
