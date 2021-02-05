# Test Tools readme

Test Tools is a mod for developers that adds a bunch of tools to directly manipulate nodes and entities. This is great for quickly testing out stuff.

Here's the list of tools:

## Remover
Removes nodes and non-player entities that you punch.

## Node Setter
Replace a node with another one.

First, punch a node you want to remember.
Then rightclick any other node to replace it with the node you remembered.

If you rightclick while pointing nothing, you can manually enter the node and param2.

## Param2 Tool
Change the value param2 of nodes.

* Punch: Add 1 to param2
* Sneak+Punch: Add 8 to param2
* Place: Subtract 1 from param2
* Sneak+Place: Subtract 8 from param2

Note: Use the debug screen (F5) to see the param2 of the pointed node.

## Falling Node Tool
Turns nodes into falling nodes.

Usage:

* Punch node: Make it fall
* Place: Try to teleport up to 2 units upwards, then make it fall

## Node Meta Editor
Edit and view metadata of nodes.

Usage:

* Punch: Open node metadata editor

## Entity Rotator
Changes the entity rotation (with `set_rotation`).

Usage:

* Punch entity: Rotate yaw
* Punch entity while holding down “Sneak” key: Rotate pitch
* Punch entity while holding down “Special” key (aka “Aux”): Rotate roll

Each usage rotates the entity by 22.5°.

## Entity Spawner
Spawns entities.

Usage:

* Punch to select entity or spawn one directly
* Place to place selected entity

## Object Property Editor
Edits properties of objects.

Usage:

* Punch object to open a formspec that allows you to view and edit properties
* Punch air to edit properties of your own player object

To edit a property, select it in the list, enter a new value (in Lua syntax)
and hit “Submit”.

## Object Attacher
Allows you to attach an object to another one.

Basic usage:
* First select the parent object, then the child object that should be attached
* Selecting an object is done by punching it
* Sneak+punch to detach selected object
* If you punch air, you select yourself

Configuration:
* Place: Increase attachment Y position
* Sneak+place: decrease attachment Y position
* Aux+place: Increase attachment X rotation
* Aux+Sneak+Rightclick: Decrease attachment X rotation

Hint: To detach all objects nearby you (including on yourself), use the
`/detach` server command.

## Object Mover
Move an object by a given distance.

Usage:
* Punch object into the direction you want to move it
* Sneak+punch: Move object towards you
* Place: Increase move distance
* Sneak+place: Decrease move distance

## Children Getter
Shows list of objects that are attached to an object (aka "children") in chat.

Usage:
* Punch object: Show children of punched object
* Punch air: Show your own children

## Entity Visual Scaler
Change visual size of entities

Usage:

* Punch entity to increase visual size
* Sneak+punch entity to decrease visual size

## Light Tool
Show light level of node.

Usage:
* Punch: Show light info of node in front of the punched node's side
* Place: Show light info of the node that you touched
