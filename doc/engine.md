# Guidelines for engine contributions

This document is meant to clarify and codify the standard
by which engine contributions may be judged in relation
to the mission statement of the Minetest project.

Minetest is a game engine and a platform for authoring,
distributing and running 3D voxel games, single and multiplayer.

## 1. The server model and API design

Minetest employs a client-server model at all times.
Even in singleplayer, a logical server
is created and the client is made to connect to it.

At the moment, a game is not allowed to send any
code for the client to execute, so all game code
is effectively executed only on the server.

The client only receives media assets and instructions on how
to render them, and only handles a rudimentary portion
of the gameplay which would be too slow to control remotely,
such as player movement and collision.

This actually allows clients to connect to servers to
discover and play games and mods without the need to
install them ahead of time, which is a unique and
valuable feature of the Minetest ecosystem,
differentiating it from other voxel game engines and clones.

Therefore, any API exposed to modders and game developers
is implemented in the form of server-side functionality,
which may then trigger action on the client environment by
means of fixed-function remote network commands.

This has some consequences for the design of engine APIs
and features. Note that introduction of server-sent scripts
in the future may relax and change some rules, but this
is what we're stuck with for now.

First of all, every feature that requires
support in the client environment adds to the amount of
individual network packets and code to support them,
as well as client code which implements the functionality.

Therefore, features that require communicating with the client
shall be designed to maximize the functionality gained from them,
and to support as many possible use cases as possible.

Second, game code is not able to control client side
capabilities without explicit API support for it.
The consequences of this are expanded upon in the following sections.

Lastly, even for features that do not require communication
with the client, consider the maintenance cost, and whether it is
possible to achieve the same goal with reasonable performance
using Lua scripts alone - if it is, it probably does not need
an engine feature. Resist the temptation to add helper functions
for every single thing, mods and libraries can do it just fine.

### 1.1. PR design checklist

Here is a list of questions you can ask yourself that can help you
triage whether or not an engine change is necessary or useful.

* Am I solving an actual problem?
* Am I just dealing with a legitimately hard computer science problem, and not a limitation?
* Can the problem be framed in a more general way?
* Is it possible to do what I want with Lua scripts?
* Is the performance of the Lua solution acceptable?
* Is this gonna be useful to others as well?
* Is the solution robust and not just a hack?

## 2. Obligation to content creators

Game developers and modders should be provided with a
reasonable, stable working environment to create their content in.

At no point should you pull the rug from under a developer by
changing this environment in an incompatible way.

New features should primarily serve the purpose of providing
content creators with more freedom. Features that restrict creative
freedom in any way are unacceptable.

## 3. Audiovisual style and artwork

While primarily hosting simple-looking, blocky voxel games, Minetest
supports a wide range of artwork styles and accomodates creators
of all skill levels, from beginner hobbyists to professionals.

Making good art for use with Minetest should be no more difficult
than in any other commonly available game engine.

After picking an art style based on the available features, an artist should
not be forced to deal with new features incompatible with their art.
New visual features should always accomodate all visual styles, providing
an opt-out for developers who have no use for them. This opt-out must
come in the form of scripting API provisions.
It is unacceptable for a creator or server admin to have to ask the user
to configure the client in a certain way.

A content creator's restrictions on presentation take precedence over
personal preference. It should not be easy for a user to unknowingly
configure their client in a way where they may get a sub-par experience,
and possibly even blame the content creator for it.
Understand that some types of artwork and some rendering features do not mix.
A model made and textured for an unlit renderer is unlikely to look
better with gouraud shading or shadows, it's more likely to look worse.

Heed the words of one dissatisfied user: (Issue [#10307](https://github.com/minetest/minetest/issues/10307), worth a read)

> When people enable additional graphics features, they expect better graphics, not worse

An implementation of a visual feature should aim for a balanced
quality and performance profile that fits a general use case and works
on a wide range of devices.

Visual features that do not work on all supported devices may be regarded
as low priority. (This does not apply to pure optimizations.)

### 3.1. Client visual settings

While the client may toggle various visual features for the sake
of personal preference and performance, games should not be
forced to utilize or anticipate those that significantly
change the way content is being rendered.

Examples of visually neutral settings:
* Screen resolution
* Maximum FPS
* Vertical sync
* Anti-aliasing
* Texture filtering
* Stereoscopy

The above settings affect the image only in minor ways,
and are safe to apply to just about any content.


Examples of non-neutral features that should be API gated:
* Direct lighting
* Realtime shadows
* Bump-mapping
* FOV changes

Above features affect visuals or gameplay in significant ways,
requiring some degree of gating in the scripting API.
It's best if the language in the config descriptions is carefully
chosen to use words such as "allow" rather than "enable", to
avoid confusion.

It does not suffice to mark a feature as "experimental" or have an explicit
"force" setting - assume that the user will enable everything you put
in front of them, even to their detriment. Don't expect every user to know
what the options actually do.

A third class of visual settings would be speedhacks, such as:
* Opaque water
* Opaque leaves
* No waving

With speedhacks, there is a clear understanding that they are sacrificing
quality for performance, and possibly breaking things in the process.
In future GUI redesigns they should probably be labeled more clearly as such.

Finally, texture packs are out of scope for these considerations,
as they are a deliberate mechanism to locally customize
the appearance of content.

## 4. Upgrading

Occasionally, it may be impossible to improve the engine without
breaking compatibility in some ways. While it is never acceptable
to break compat without providing some way to restore the original
conditions, it may sometimes be okay to require some action from
a mod or game author to maintain compatibility with subsequent
engine versions. However, significant changes to existing APIs
should only happen in a major release. Scripts should at least
compile and run unmodified between minor releases.

The standard of reasonable burden on a developer when upgrading
to a new engine version is outlined in the following section.

### 4.1. Reasonable upgrade burden

The amount of work required from a game developer or modder
to upgrade their scripts must be reasonable, and the old
functionality must not be degraded in noticeable ways.
Features must not consume any resources of the client
or the server after being disabled by game code.

Examples of reasonable burdens:
* Utilizing a new API function in a few places
* Utilizing a new object or node flag
* Re-exporting or converting some assets (without loss of data or function)

Examples of unreasonable burdens and effects:
* Having to rewrite huge amounts of code
* Having to send a lot more network traffic than before
* Emulating old functionality in suboptimal ways
* Relying on a .conf entry rather than an API function

The standard of these burdens may be more relaxed in
a major release (6.0, 7.0, etc) but under no circumstances
may the new API heavily favor one use case over another.

## 5. Error recovery

In the event that a PR is merged without consideration for the
needs of content creators, or if it accidentally breaks any of
the above promises:
* The report is accepted and tagged as a bug
* The bug becomes a blocker for the next release
* The bug is fixed in a timely manner (ideally by the one who caused it)
