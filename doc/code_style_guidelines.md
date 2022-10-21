# Code style guidelines

This is the coding style used for C/C++ code. Also see the Lua code style guidelines.

The coding style is based on the [Linux kernel code style](https://www.kernel.org/doc/html/latest/process/coding-style.html). Much of the existing code doesn't follow the current code style guidelines, do not try to replicate that. Use your best judgment for C++-specific syntax.

Currently, the code uses C++14. Do not use features that depend on more recent versions.


## Spelling

Use American English, but avoid idioms that may be difficult to understand by non-native speakers.


## Function declarations

In case your function parameters don't fit within the defined line length, use the following style.
Indention for continuation lines is **exactly** two tabs:

```cpp
void some_function_name(type1 param1, type2 param2, type3 param3,
		type4 param4, type5 param5, type6 param6, type7 param7)
{
	...
}
```

Sometimes with complex function declarations, it might be messy to define as many parameters as possible on the same line.
This is acceptable too (and currently used in some places):

```cpp
void some_function_name(
		const ReallyBigLongTypeName &param1,
		ReallyBigLongTypeName *param2,
		void *param3,
		size_t param4,
		const void *param5,
		size_t param6)
{
	...
}
```

No more than 7 parameters allowed (except for constructors).


## Spaces

<span style="color: red">Do **not** use spaces to indent.</span>

Try to stay under 6 levels of indentation.

Add spaces between operators so they line up when appropriate (don't go overboard). For example:

```cpp
np_terrain_base   = settings->getNoiseParams("mgv6_np_terrain_base");
np_terrain_higher = settings->getNoiseParams("mgv6_np_terrain_higher");
np_steepness      = settings->getNoiseParams("mgv6_np_steepness");
np_height_select  = settings->getNoiseParams("mgv6_np_height_select");
...
bool success =
		np_terrain_base  && np_terrain_higher && np_steepness &&
		np_height_select && np_trees          && np_mud       &&
		np_beach         && np_biome          && np_cave;
```
The above code looks really nice.

Separate different parts of functions with newlines for readability.

Separate functions by two newlines (not necessary, but encouraged).

Use a space after `if`, `else`, `for`, `do`, `while`, `switch`, `case`, `try`, `catch`, etc.

When breaking conditionals, indent following lines of the conditional with two tabs and the statement body with one tab. For example:

```cpp
for (std::vector<std::string>::iterator it = strings.begin();
		it != strings.end();
		++it) {
	*it = it->substr(1, 1);
}
```

Align backslashes for multi-line macros with spaces:

```cpp
#define FOOBAR(x) do { \
	int __temp = (x);  \
	foo(__temp);       \
} while (0)
```


## Bracing and indentation

### `if` statements

This rule has already been explicitly stated in the [Linux kernel code style](https://www.kernel.org/doc/html/latest/process/coding-style.html) from which this code style inherits, but it will be repeated here:

<span style="color: red">**Putting the body of an `if` statement on the same line as the condition is strictly prohibited.**</span>

Example:
```cpp
if (foobar < 3) foobar = 45;      // Bad
(foobar < 3 && (foobar = 45));    // Bad
```

Violating this rule will result in **instant rejection**.

Examples of good if statement wordings:
```cpp
if (foobar < 3)
	foobar = 45;

if (foobar < 6) {
	foobar = 62;
	return;
}
```

### Nested for loop exception

Special exception to the standard bracing/indent rules for nested loops:
If a nested loop iterates over a set of coordinates, it is permitted to omit the braces for all but the innermost loop and keep the outer loops at the same indentation level, like so:
```cpp
for (s16 z = pmin.Z; z <= pmax.Z, z++)
for (s16 y = pmin.Y; y <= pmax.Y; y++)
for (s16 x = pmin.X; x <= pmax.X; x++) {
	// ... do stuff here ...
}
```


## Do not be too C++y

Avoid passing non-`const` references to functions.

Don't use initializer lists unless absolutely necessary (initializing an object inside a class, or initializing a reference).

Try to minimize the use of exceptions.

Avoid operator overloading like the plague.

Avoid templates unless they are very convenient.

Usage of macros is not discouraged, just don't overdo it [like X.org](http://cgit.freedesktop.org/xorg/xserver/tree/randr/rrscreen.c?id=01e18af17f8dc91451fbd0902049045afd1cea7e#n325). It's better to use inline functions or lambdas instead.


## Classes

**Class names are *PascalCase*, method names are *camelCase*.**

Don't put actual code in header files, unless it's a 4-liner, an inline function, or part of a template.

Class definitions should go in header files.

Substantial methods (over 4 lines) should be defined outside of the class definition.

Functions not part of any class should use `lowercase_underscore_style()`.


## Comments

Doxygen comments are acceptable, but **please** put them in the header file.

Don't make uninformative comments like this:

```cpp
// Draw "Loading" screen
draw_load_screen(L"Loading...", driver, font);
```

Add comments to explain a non-trivial but important detail about the code, or explain behavior that is not obvious.

For comments with text, be sure to add a space between the text and the comment tokens:

```cpp
DoThingHere();  // This does thing      /* <--- yes! */
DoThingHere();  /* This does thing */   /* <--- yes! */

DoThingHere();  //This does thing       /* <--- no! */
DoThingHere();  /*This does thing*/     /* <--- no! */
DoThingHere();//This does thing         /* <--- no! */
```


## Use STL, avoid Irrlicht containers, and no, Boost will not even be considered, so forget it

In general, adding new dependencies is considered serious business.

We are using C++14; Boost will never be an option.


## Don't let things get too large

**Try to keep lines under 95 characters.** It's okay if it goes over by a few, but do not exaggerate. (Note that this column count assumes 4-space indents.)

Functions should not have over 200 lines of code – if you are concerned with having to pass too many parameters to child functions, make whatever it is into a class.

Don't let files get too large (over 1500 lines of code). Currently, existing huge files (`game.cpp`, `server.cpp`, …) are in the slow process of being cleaned up.


## Files

Files should be named using *snake_case* style.

Files should have includes for everything that they depend on. Don't depend on, eg, `"util/numeric.h"` including `<string>`!

Uniqueness when compiling headers is ensured by using `#pragma once`. ([Accepted by all coredevs](https://github.com/minetest/minetest/issues/6259))

```cpp
#pragma once

#include <string>

class Foo {
};

```

All files should include the appropriate license header.


## Miscellaneous

<span style="color: red">Do **not** use `or`, use `||`.</span>

Set pointer values to `nullptr` (C++11), not 0.

When using floats, add the `f` suffix, e.g. `float k = 0.0f;` and not `float k = 0.0;`.

Avoid non-ASCII characters in source files. Other UTF-8 characters may (only) be used in string literals and comments where ASCII would worsen readability.

Use of Hungarian notation is very limited. Scope specifiers such as `g_` for globals, `s_` for statics, or `m_` for members are allowed. The prefix `m_` is discouraged for public members in newer code as it is a part of the class' interface, but sometimes needed for consistency when adding a member to older code.

Use *snake_case* for local variables, not *camelCase*.

Don't use distracting and unnecessary amounts of object-oriented abstraction. See [Terasology](https://github.com/MovingBlocks/Terasology) as an example of what not to do.

Don't add unnecessary design patterns to your code, such as factories/providers/sources.

In `switch-case` statements, add `break` to the last case and to the `default` case.

In `if-else` statements, put the code which is more likely to be executed first.

For consistency, use American English where spellings differ (e.g. use "color", not "colour").
