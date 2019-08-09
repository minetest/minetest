# Minetest Direction Document



## 0. Preface

### 0.1. Introduction

This document attempts to set out Minetest's philosophy, aims, and objectives.

Please note that Minetest is developed by a community, and not everyone in the community
holds the same views as to what Minetest should be. Therefore, defining a common direction is
very difficult.

### 0.2. Definitions

* `Minetest` is the game engine.
* `Minetest Game` is the default game which comes with the engine.
* The `Minetest Project` refers to the combination of the above, the development team, and supporting tools.

### 0.3. Other documents

Please read celeron55's document: http://c55.me/blog/?p=1491

Also see the roadmaps and manifestos of core developers on the forums: https://forum.minetest.net/viewforum.php?f=7


## 1. Mission Statement

Minetest provides both a game engine, which allows the creation of voxel games using Lua, and
a content distribution platform. Players can connect to servers with their own game play, and
easily install content through the Minetest client.

## 2. Goals

* **Stability and performance**, especially on older devices.
* **Keep the engine lightweight**. Support a large amount of gameplay by pushing code and functionality out to Lua,
  rather than having a complex core. Keeping the core as small as possible makes it
  easier to test and more flexible.
* **Keep the engine as universal as possible**. It's worth comparing everything that goes
  into the engine for how well it suits a gravitation-less space game without ground surface.
  If everything is either disableable or suitable for such, then it gives much more freedom
  for people to experiment and create interesting things, even in ground-based worlds.
* **Make polishing easy**. It shouldn't be made unnecessarily hard to make polished content
  even while most content isn’t polished.

## 3. Target Games

It's important to limit the scope of the engine to something which is managable.

* Games where players are in 1st or 3rd person.
* Games with roughly meter cubed cubes.
