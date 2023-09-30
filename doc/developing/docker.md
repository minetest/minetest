# Developing minetestserver with Docker

Docker provides an easy cross-platform solution to create a development environment. The advantage of this method is that it does not require you to install any development tools and libraries on your machine besides git and docker. This is a short guide describing how to set up a development environment for minetestserver; people unfamiliar with docker may also want to refer to the [Docker Documentation](https://docs.docker.com//) which is well written.

## Creating an image

The first step is to create a development image of minetestserver:
```bash
docker buildx build --target dev -t minetest-dev:0 .
```
The `--target dev` option instructs docker to build the intermediate development image, and the `-t minetest-dev:0` option tags the image so we can refer to it by that name.

## Opening a new container

Once an image has been created, a new container can be opened, with the source code mounted into it:
```bash
docker run -it \
  --mount type=bind,source=/home/bob/minetest,target=/minetest \
  minetest-dev:0
```
The `-it` runs the container interactively and puts you into a terminal in the container. The source and target of the bind mount should point to the directory on the host machine, and the directory in the container, respectively.

### For VSCode users

If you install the development container extension from the VSCode marketplace, you can attach VSCode to the running container, and open the source in VSCode to work with it as you would any other project. Note that extensions must be installed in the container from the extensions tab once the container has been attached if you wish to use them. For more information, see the VSCode documentation on [Developing inside a Container using Visual Studio Code Remote Development](https://code.visualstudio.com/docs/devcontainers/containers#:~:text=The%20Visual%20Studio%20Code%20Dev%20Containers%20extension%20lets,advantage%20of%20Visual%20Studio%20Code%27s%20full%20feature%20set.).
