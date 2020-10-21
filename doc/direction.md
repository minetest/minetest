# Minetest Direction Document

## 1. Background

In order to give context to this document, you should consider reading the
following resources.

* celeron55 has published his opinions on the development of Minetest in these places:
  * [What is Minetest?](http://c55.me/blog/?p=1491)
  * [celeron55's roadmap](https://forum.minetest.net/viewtopic.php?t=9177)
  * [A clear mission statement for Minetest is missing](https://github.com/minetest/minetest/issues/3476#issuecomment-167399287)
* Also see the roadmaps and manifestos of core developers on the forums:
  https://forum.minetest.net/viewforum.php?f=7


## 2. Roadmap

These are the current medium term goals for Minetest development.
Pull-requests that address these goals will be prioritised.

This roadmap was created after a roadmap brainstorm in a
[GitHub issue](https://github.com/minetest/minetest/issues/10461).
The goals aren't in any particular order.

### 2.1 Rendering/Graphics improvements

The implementation of Minetest's renderer is questionable, in terms of
correctness.
Work is needed to fix bugs and glitches, such as transparency sorting.
There are also performance issues (particles, general view distance).

Once these issues are fixed, more features can be added such as water shaders,
shadows, and improved lighting.

Irrlicht may be a limiting factor when it comes to implementing this goal, in
which case it may need to be maintained internally or removed.

### 2.2 Internal code refactoring

In order to ensure sustainable development, Minetest's code needs to be refactored
and improved. This will remove code rot and allow for more efficient development.

### 2.3 UI Improvements

A [formspec replacement](https://github.com/minetest/minetest/issues/6527) is
needed, to make GUIs better and easier to create. This replacement could also
be a replacement for HUDs, allowing for a unified API.

A [new mainmenu](https://github.com/minetest/minetest/issues/6733) is needed to
improve user experience. First impressions matter, and the current main menu
doesn't do a very good job at selling Minetest or explaining what it is.

### 2.4 Object and entity improvements

There are still a significant number of issues with objects. Collisions,
performance, API convenience, and discrepancies between players and entities.
