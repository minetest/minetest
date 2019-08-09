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

celeron55 has published his opinions on the development of Minetest in these places:

* What is Minetest? - http://c55.me/blog/?p=1491
* Roadmap - https://forum.minetest.net/viewtopic.php?t=9177
* A clear mission statement for Minetest is missing - https://github.com/minetest/minetest/issues/3476#issuecomment-167399287

Also see the roadmaps and manifestos of core developers on the forums: https://forum.minetest.net/viewforum.php?f=7

---------------------------

## 1. Mission Statement

Minetest provides both a game engine, which allows the creation of voxel games
using Lua, and a content distribution platform. Players can connect to servers
with their own gameplay, and easily install content through the Minetest client.
The goal is to provide an easy to learn and use, but still powerful system which
allows to easily and rapidly create, extend and modify voxel games.

## 2. Aims

* **Stability and performance**,
		especially on older devices.
* **Keep the engine as universal as possible**.
		It's worth comparing everything that goes into the engine for how well
		it suits a gravitation-less space game without ground surface.
		If everything is either disableable or suitable for such, then it gives
		much more freedom for people to experiment and create interesting things,
		even in ground-based worlds.
* **Keep the engine lightweight**.
		Support a large amount of gameplay by pushing code and functionality out
		to Lua, rather than having a complex core. Keeping the core as small as
		possible makes it easier to test and more flexible.
* **Prefer simplicity**.
		Try to be simple in both API and implementing features.
* **Self-contained**.
		Avoids relying on third party behaviour. For example, there is no
		central account system and the serverlist isn't actually needed to play online.
* **Multi-platform**. Essential features should work on all platforms.
* **Gameplay over graphics**.
* **Maintain fork-friendliness**.

## 3. Scope of Game Engine

The engine is designed for games which fit these rough limitations:

* Players are in 1st or 3rd person.
* Roughly meter sized cubes.

## 4. Future Work

See https://dev.minetest.net/TODO
