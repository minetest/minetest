README.md for the Minetest Core Docs
====================================

# Technical documentation of the Minetest Engine

An introductory README document to the comprised MT core docs folder.

The folder `doc` seeks to comprise the **Minetest Documentation** for
the MT core development within the Minetest (MT) project and its
respective Git repository at GitHub.
<https://github.com/minetest/minetest>.

The documents comprised should be including (but not necessarily be
limited to) the ones listed in alphabetical order in the following:

# Short List

Technical documentation of the Minetest Engine (Short List)

This folder contains documentation to the Minetest Engine that both the
MT Developers and the MT Modders may be interested in respectively. The
documents filed in `doc` are short listed below by brief description.

* `mkdocs/*`, `main_page.dox` and `Doxyfile.in`: documentation generator

* `README.android`: gameplay and compiling instructions for Android

* `breakages.md`: breaking changes for the next major version

* `builtin_entities.txt`: list of builtin Lua entities

* `client_lua_api.txt`: an (unstable) doc to the MT client modding API

* `direction.md`: roadmaps of further development

* `fst_api.txt`: the Forkspec toolkit

* `lgpl-2.1.txt`: GNU Lesser General Public License, Version 2.1

* `lua_api.txt`: the server-side Lua API.

* `menu_lua_api.txt`: the Lua API calls available to the MT main menu

* `minetest.6` and `minetestserver.6`: man pages respectively

* `mod_channels.png`: image file on the concept of mod channels

* `protocol.txt`: early draft of the MT communications protocol

* `texture_packs.txt` : structure of a texture pack

* `world_format.txt`: structure of a world format definiton

The below Content List provides a more elaborate and navigable list:

# Content List

Technical documentation of the Minetest Engine (Content List)

## The Minetest Major Breakages List

This document contains a list of breaking changes to be made in the next
major version.

[File: breakages.md](breakages.md)

## The Minetest Description of Builtin Entities

Minetest registers two entities by default: Falling nodes and dropped
items. This document describes how they behave and what you can do with
them.

[File: builtin_entities.txt](builtin_entities.txt)

## The Minetest Lua Client Modding API Reference

Minetest is a game engine inspired by Minecraft, Infiniminer and others.
It's licensed under the GNU LGPL 2.1, with artwork generally covered by
CC BY-SA 3.0. There are two major parts to the system, the first being a
core engine written in C++, the second being a modding API that exposes
useful functions to a Lua environment.

This reference defines the modding API of functions to the Minetest Lua
environment.

* More information at <http://www.minetest.net/>

* Developer Wiki: <http://dev.minetest.net/>

* Getting started with Lua: <https://www.lua.org/start.html>

* Lua 5.1 Reference Manual: <https://www.lua.org/manual/5.1/>

* The Minetest **Wiki for players** is located at
<https://wiki.minetest.net/>

[File: client_lua_api.txt](client_lua_api.txt)

## The Minetest Direction Document

The Minetest direction document contains:

1. Long-term Roadmap

2. Medium-term Roadmap

[File: direction.md](direction.md)

## Doxyfile Configuration Input

This config-file is used to generate documentation from source code.

Doxygen is a standard tool for generating documentation from annotated
C++ sources.

* More information at <https://doxygen.nl/manual/starting.html>

[File: Doxyfile.in](Doxyfile.in)

## The Formspec Toolkit API

The MT Formspec toolkit is a set of functions to create basic UI
elements.

[File: fst_api.txt](fst_api.txt)

## The License Text

Text of the LGPL-2.1 License Agreement.

* More information at
<https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html>

[File: lgpl-2.1.txt](lgpl-2.1.txt)

## The Minetest Lua Modding API Reference

* More information at <http://www.minetest.net/>

* Developer Wiki: <http://dev.minetest.net/>

* (Unofficial) Minetest Modding Book by rubenwardy:
<https://rubenwardy.com/minetest_modding_book/>

[File: lua_api.txt](lua_api.txt)

## The Minetest engine internal documentation header

This Doxy file is used to generate documentation from source code.

[File: main_page.dox](main_page.dox)

## The Minetest Lua Mainmenu API Reference

The main menu is defined as a formspec by Lua in ../builtin/mainmenu/
Description of formspec language to show your menu is in lua_api.txt

[File: menu_lua_api.txt](menu_lua_api.txt)

## The Minetest Manual

GROFF template to generate the `minetest.6` man page for games on a
Unixoid OS.

* More information at
<https://www.linuxhowtos.org/System/creatingman.htm>

[File: minetest.6](minetest.6)

## The Minetest Server Manual

GROFF template to generate the `minetestserver.6` man page for games on
a Unixoid OS.

* More information at
<https://www.linuxhowtos.org/System/creatingman.htm>

[File: minetestserver.6](minetestserver.6)

## The MT Server Client Channel Schematics

Graphical presentation in PNG format on the MT server client channel
schematics.

[File: mod_channels.png](mod_channels.png)

## The Minetest Protocol Description

Description of the MT protocol used on the MT server client channel.

CAVEAT: This might be a somewhat incomplete, early DRAFT document.

[File: protocol.txt](protocol.txt)

## Minetest: Android Version

The Android port of MT doesn't support everything you can do on PC due
to the limited capabilities of common devices.

[File: README.android](README.android)

## The README.md for the Minetest Core Docs

A more introductory README document (this document) to the comprised MT
core documentation.

[File: README.md](README.md) (i.e. this document)

## The Minetest Texture Pack Reference

Texture packs allow you to replace textures provided by a mod with your
own textures.

[File: texture_packs.txt](texture_packs.txt)

## The Minetest World Format

This describes the MT world format, files and objects of the MT block
serialization.

CAVEAT: The block serialization version does not fully specify every
aspect of this format; if compliance with this format is to be checked,
one should first investigate if the files and data indeed follow it.

[File: world_format.txt](world_format.txt)

- - -

# Further Information

## Minetest

Minetest (usually abbreviated as MT) is an open source voxel game
engine. Play one of our many games, mod a game to your liking, make your
own game, or play on a multiplayer server.

Available for Windows, macOS, GNU/Linux, FreeBSD, OpenBSD, DragonFly
BSD, and Android.

* More information at <https://www.minetest.net>

## How to contribute or how to get involved?

* More information at <https://www.minetest.net/get-involved/>

## How is Minetest developed?

### The Process

Minetest (MT) is developed and maintained by a group of volunteers
called the [core team](https://github.com/orgs/minetest/people),
consisting of a bunch of people who are trusted to keep Minetest
progressing in good condition. The core team is formed of people who
have made great [contributions](https://www.minetest.net/credits/) to
Minetest. Contributions are approved if two members of the core team
agree on them.

All development and decisions are made in public, on
[GitHub](https://github.com/minetest) and [Internet Relay Chat
(IRC)](https://wiki.minetest.net/IRC). Meetings are occasionally held on
IRC, with [plans and notes](https://dev.minetest.net/Meetings) made
public.

The core team is best contacted on [IRC](https://wiki.minetest.net/IRC)
at `#minetest-dev @ irc.libera.chat`.

For more information, take a look at [all the rules regarding to
development](https://dev.minetest.net/All_rules_regarding_to_development
).

### Project Structure

Minetest is distributed as an engine, combined with a couple of games.
Upstream repositories can be found at <https://github.com/minetest/>.

  * **The engine** (core) is the base for everything. The programming
  language C++ is used for housekeeping and performance-critical stuff,
  the extension programming language Lua for extensible things.

  * **Games** define game content: nodes, entities, textures, meshes,
  sounds and custom behavior implemented in Lua. Games and respective
  additions to games consist of mods that plug into the engine using the
  [Modding API](https://dev.minetest.net/).

Contributors should be familiar with this project structure. For more
information, see the [terminology](https://dev.minetest.net/Terminology)
or the [engine overview](https://dev.minetest.net/Engine) developer wiki
pages.

### Roadmaps and Future Plans

As an open source project developed by volunteers, Minetest is mostly
iteratively developed rather than formally planned. However, there are
some overarching goals, both medium-term and long-term, that have been
agreed upon by core developers: [Roadmap](direction.md)

## celeron55

**celeron55** (founder)

He is the original creator of Minetest, and hosts the forums and wikis.
While he isn't involved in active development, he plays an advisory role
and breaks ties.

## How to Donate?

* More information can be found under **Donate** near the bottom page at
<https://www.minetest.net/get-involved/>

- - -

No spirit was harmed in the making of this document.
