# rattler-build recipe

[rattler-build](https://prefix-dev.github.io/rattler-build/latest/) recipes.

If you change the recipe.yaml in master, a
[GitHub workflow](../.github/workflows/package.yaml) should rebuild and upload the packages.

Note you should bump the version number or build number whenever you want a new version
released, otherwise the upload will fail because that version already exists.

## Local usage

One-time, install rattler-build:
```sh
pixi global install rattler-build
```

Then to rebuild:
```sh
rattler-build build --recipe-dir=./rattler-build-recipe -c conda-forge
ls -lh output/{noarch,linux-64}
```

