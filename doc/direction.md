# Minetest Direction Document

---------------------------

## 0. Preface

### 0.1. Introduction

This document attempts to set out Minetest's philosophy, aims, and objectives.

Please note that Minetest is developed by a community, and not everyone in the community
holds the same views as to what Minetest should be.

### 0.2. Definitions

* `Minetest` is the game engine.
* `Minetest Game` is the default game which comes with the engine, and is not covered by this document.
* The `Minetest Project` refers to the combination of the above, the development team, and supporting tools.

### 0.3. Other documents

In order to understand this document, it's important to get context.

celeron55 has published his opinions on the development of Minetest in these places:

  * What is Minetest? - http://c55.me/blog/?p=1491
  * Roadmap - https://forum.minetest.net/viewtopic.php?t=9177
  * A clear mission statement for Minetest is missing - https://github.com/minetest/minetest/issues/3476#issuecomment-167399287

Also see the roadmaps and manifestos of core developers on the forums: https://forum.minetest.net/viewforum.php?f=7

---------------------------

## 1. Mission Statement

Minetest provides both a game engine, which allows the creation of voxel games using Lua, and
a content distribution platform. Players can connect to servers which provide their own gameplay,
and easily install content through the Minetest client. The goal is to provide an easy to learn and use,
but still powerful system which allows to easily and rapidly create, extend and modify voxel games.

## 2. Aims

1. **Stability and performance**,
		especially on older devices.
2. **Keep the engine as universal as possible**.
		It's worth comparing everything that goes into the engine for how well
		it suits a gravitation-less space game without ground surface.
		If everything is either disableable or suitable for such, then it gives
		much more freedom for people to experiment and create interesting things,
		even in ground-based worlds.
3. **Keep the engine lightweight**.
		Support a large amount of gameplay by pushing code and functionality out
		to Lua, rather than having a complex core. Keeping the core as small as
		possible makes it easier to test and more flexible.
4. **Prefer simplicity**.
		Try to be simple in both API and implementing features.
5. **Self-contained**.
		Avoids relying on third party behaviour. For example, there is no
		central account system and the serverlist isn't actually needed to play online.
6. **Multi-platform**. Essential features should work on all platforms.
7. **Gameplay over graphics**.
8. **Maintain fork-friendliness**.

## 3. Scope of Game Engine

The engine is designed for games which fit these rough limitations:

* The world consists of a 3d grid of discrete positions, the distance between each position
  is called a unit length.
* Each position is filled by exactly one node. Positions that appear to be empty are actually filled with airlike nodes.
* The visible size of a unit length may change according to the game's requirements, but the engine does
  not aim to promise good performance for visible sizes far deviating the default.
* Games will tend to define the unit length distance a meaning, for example, Minetest Game defines.

## 4. Future Work

* **Improving flexibility**.
		To fulfill aim 2, development will seek to push functionality to Lua and increase the flexibility
		and potential.
* **Improved graphics and rendering performance**
		Development will seek to improve the performance of rendering, to increase FPS on both old and new
		devices. New graphical features may be added if they do not jeapordize performance when not used.
* **Improve code structure and maintainability**
		Development will seek to improve the maintainability of code through refactoring.

Also see https://dev.minetest.net/TODO
