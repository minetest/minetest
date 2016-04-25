
MOBS REDO for MINETEST

Built from PilzAdam's original Simple Mobs with additional mobs by KrupnoPavel, Zeg9, ExeterDad and AspireMint.


This mod contains the API only for adding your own mobs into the world, so please use the additional modpacks to add animals, monsters etc.


https://forum.minetest.net/viewtopic.php?f=11&t=9917


Changelog:

- 1.29- Split original Mobs Redo into a modpack to make it easier to disable mob sets (animal, monster, npc) or simply use the Api itself for your own mod
- 1.28- New damage system added with ability for mob to be immune to weapons or healed by them :)
- 1.27- Added new sheep, lava flan and spawn egg textures.  New Lava Pick tool smelts what you dig.  New atan checking function.
- 1.26- Pathfinding feature added thanks to rnd, when monsters attack they become scary smart in finding you :) also, beehive produces honey now :)
- 1.25- Mobs no longer spawn within 12 blocks of player or despawn within same range, spawners now have player detection, Code tidy and tweak.
- 1.24- Added feature where certain animals run away when punched (runaway = true in mob definition)
- 1.23- Added mob spawner block for admin to setup spawners in-game (place and right click to enter settings)
- 1.22- Added ability to name tamed animals and npc using nametags, also npc will attack anyone who punches them apart from owner
- 1.21- Added some more error checking to reduce serialize.h error and added height checks for falling off cliffs (thanks cmdskp)
- 1.20- Error checking added to remove bad mobs, out of map limit mobs and stop serialize.h error
- 1.19- Chickens now drop egg items instead of placing the egg, also throwing eggs result in 1/8 chance of spawning chick
- 1.18- Added docile_by_day flag so that monsters will not attack automatically during daylight hours unless hit first
- 1.17- Added 'dogshoot' attack type, shoots when out of reach, melee attack when in reach, also api tweaks and self.reach added
- 1.16- Mobs follow multiple items now, Npc's can breed
- 1.15- Added Feeding/Taming/Breeding function, right-click to pick up any sheep with X mark on them and replace with new one to fix compatibility.
- 1.14- All .self variables saved in staticdata, Fixed self.health bug
- 1.13- Added capture function (thanks blert2112) chance of picking up mob with hand; net; magic lasso, replaced some .x models with newer .b3d one's
- 1.12- Added animal ownership so that players cannot steal your tamed animals
- 1.11- Added flying mobs (and swimming), fly=true and fly_in="air" or "deafult:water_source" for fishy
- 1,10- Footstep removed (use replace), explosion routine added for exploding mobs. 
- 1.09- reworked breeding routine, added mob rotation value, added footstep feature, added jumping mobs with sounds feature, added magic lasso for picking up animals
- 1.08- Mob throwing attack has been rehauled so that they can damage one another, also drops and on_die function added
- 1.07- Npc's can now be set to follow player or stand by using self.order and self.owner variables
- beta- Npc mob added, kills monsters, attacks player when punched, right click with food to heal or gold lump for drop
- 1.06- Changed recovery times after breeding, and time taken to grow up (can be sped up by feeding baby animal)
- 1.05- Added ExeterDad's bunny's which can be picked up and tamed with 4 carrots from farming redo or farming_plus, also shears added to get wool from sheep and lastly Jordach/BSD's kitten
- 1.04- Added mating for sheep, cows and hogs...  feed animals to make horny and hope for a baby which is half size, will grow up quick though :)
- 1.03- Added mob drop/replace feature so that chickens can drop eggs, cow/sheep can eat grass/wheat etc.
- 1.02- Sheared sheep are remembered and spawn shaven, Warthogs will attack when threatened, Api additions
- 1.01- Mobs that suffer fall damage or die in water/lava/sunlight will now drop items
- 1.0 - more work on Api so that certain mobs can float in water while some sink like a brick :)
- 0.9 - Spawn eggs added for all mobs (admin only, cannot be placed in protected areas)...  Api tweaked
- 0.8 - Added sounds to monster mobs (thanks Cyberpangolin for the sfx) and also chicken sound
- 0.7 - mobs.protected switch added to api.lua, when set to 1 mobs no longer spawn in protected areas, also bug fixes
- 0.6 - Api now supports multi-textured mobs, e.g oerkki, dungeon master, rats and chickens have random skins when spawning (sheep fix TODO), also new Honey block
- 0.5 - Mobs now float in water, die from falling, and some code improvements
- 0.4 - Dungeon Masters and Mese Monsters have much better aim due to shoot_offset, also they can both shoot through nodes that aren't walkable (flowers, grass etc) plus new sheep sound :)
- 0.3 - Added LOTT's Spider mob, made Cobwebs, added KPavel's Bee with Honey and Beehives (made texture), Warthogs now have sound and can be tamed, taming of shaved sheep or milked cow with 8 wheat so it will not despawn, many bug fixes :)
- 0.2 - Cooking bucket of milk into cheese now returns empty bucket
- 0.1 - Initial Release
