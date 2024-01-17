from setuptools import find_packages, setup

setup(
    name="minetester",
    version="0.1.0",
    description="A Gym environment for minetest",
    author="Simon Boehm",
    author_email="simon@astera.org",
    packages=find_packages(),
    package_data={
        "minetester": ["../src/network/proto/remoteclient.capnp"],
    },
    install_requires=[
        "numpy",
        "pygame",
        "gymnasium",
        "pyzmq",
    ],
)
