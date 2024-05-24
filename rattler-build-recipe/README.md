# rattler-build recipe

[rattler-build](https://prefix-dev.github.io/rattler-build/latest/) recipes.

Usage:

One-time, install rattler-build:
```sh
pixi global install rattler-build
```

Then to rebuild, edit the recipe to bumb the verison and then:
```sh
rattler-build build --package-format=conda --recipe-dir=./rattler-build-recipe -c conda-forge -c ./output
ls -lh output/{noarch,linux-64}
```

Then you need to upload the .conda files to
https://prefix.dev/channels/obelisk-public and / or
https://prefix.dev/channels/obelisk. We should be able to automate that, but for now
just `scp` the files to your laptop and upload them using the web UI.
