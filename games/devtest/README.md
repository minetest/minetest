# Development Test (devtest)

This is a basic testing environment that contains a bunch of things to test the engine, but it could also be used as a minimal testbed for testing out mods.

## Features

* Basic nodes for mapgen
* Basic, minimal map generator
* Lots of example nodes for testing drawtypes, param2, light level, and many other node properties
* Example entities
* Other example items
* Formspec test (via `/test_formspec` command)
* Automated unit tests (disabled by default)
* Tools for manipulating nodes and entities, like the "Param2 Tool"

## Getting started

Basically, just create a world and start. A few important things to note:

* Items are gotten from the “Chest of Everything” (`chest_of_everything:chest`)
* When you lost your initial items, type in `/stuff` command to get them back
* By default, Creative Mode activates infinite node placement. This behavior can be changed with the `devtest_infplace` setting
* Use the `/infplace` command to toggle infinite node placement in-game
* Use the Param2 Tool to change the param2 of nodes; it's useful to experiment with the various drawtype test nodes
* Check out the game settings and server commands for additional tests and features

Confused by a certain node or item? Check out for inline code comments. The usages of most tools are explained in their tooltips.

### Example tests

* You can use this to test what happens if a player is simultaneously in 2 nodes with `damage_per_second` but with a different value.
* Or use the Falling Node Tool on various test nodes to see how they behave when falling.
* You could also use this as a testbed for dependency-free mods, e.g. to test out how your formspecs behave without theming.

## Random notes

* Textures of drawtype test nodes have a red dot at the top left corner. This is to see whether the textures are oriented properly

## Design philosophy

This should loosely follow the following principles:

* Engine testing: The main focus of this is to aid testing of *engine* features, such as mapgen or node drawtypes
* Mod testing: The secondary focus is to help modders as well, either as a minimal testbed for mods or even as a code example
* Minimal interference: Under default settings, it shall not interfere with APIs except on explicit user wish. Non-trivial tests and features need to be enabled by a setting first
* Convenience: Have various tools to make usage easier and more convenient
* Reproducing engine bugs: When an engine bug was found, consider creating a test case
* Clarity: Textures and names need to be designed to keep different things clearly visually apart at a glance
* Low loading time: It must load blazing-fast so stuff can be tested quickly

