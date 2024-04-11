# rattler-build recipe

[rattler-build](https://prefix-dev.github.io/rattler-build/latest/) recipes.

Usage:

```sh
rattler-build build --package-format=conda --recipe-dir=./rattler-build-recipe -c conda-forge -c ./output
ls -lh output/linux-64
```

Then you need to upload the .conda files to
https://prefix.dev/channels/obelisk-public and / or
https://prefix.dev/channels/obelisk. We should be able to automate that, but for now
just `scp` the files to your laptop and upload them using the web UI.
