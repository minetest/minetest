# Custom C Libraries In Minetest


## What we'll be learning:

In this micro tutorial, you will learn how to create a small C library and minetest mod.


## How is this useful?

This is useful for creating things like: Chat relays, hyper async HTTP relay pools, custom multithreaded mod elements, advanced thread pool calculations, and whatever else you can think of.

This tutorial has been created to guide developers into the right direction as a starting point.


## Preliminary step:

As this is outside of the built in safety mechanisms of minetest, you must disable mod security to allow the ``require`` keyword to be utilized.


## The C code:

This is what our desired code will do:

```c
int poll_number()
{
  return 1;
}
```

The lua engine functions a bit differently than the regular C code. It is a virtual machine, so it has it's own ruleset.

Here, is equal code we will utilize in our library:

```c
int luaopen_yourlib(lua_State *L) {
  lua_newtable(L);
  lua_pushfstring(L, "poll_number");
  lua_pushcfunction(L, poll_number);
  lua_settable(L, 1);
}
```

As you can see, it is a bit more verbose. The intrinsics will not be explained in this tutorial.

To read further into this, here are a few resources:

https://www.lua.org/pil/24.html

https://nachtimwald.com/2014/07/12/wrapping-a-c-library-in-lua/

https://www.oreilly.com/library/view/creating-solid-apis/9781491986301/ch01.html

https://pgl.yoyo.org/luai/i/_


## The Lua code (minetest mod):

```lua
require("yourlib").poll_number()

print(poll_number())
```

As you can see, the lua side is much simpler.