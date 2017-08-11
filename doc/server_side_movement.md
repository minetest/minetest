# Server side movement validation

(This document is expected to be removed before the server side movement
validation is merged.)

## Start

The entire system starts when the server sends the first player position and
requests an "action log". The action log protocol / format is versioned; the
server sends the highest version number of action logs that it supports, or 0
if no action log is required. From this point on, the client sends the action
log in the highest version that it supports itself and that is less or
equal to what the server supports. The version is always part of the action
log packet.

The server also sends a motion model request. This consists of a numeric 8-bit
identifier of the model and an 8-bit version. Minetest currently has one motion
model in one version, so these two numbers should last a while.

## Action Log

The client starts collecting action log items. Whenever the client sends its
position, speed and orientation, it also sends an action log. The action log
basically consists of the controls and duration that the client uses during the
`applyControl` method. The remaining physics calculations are *not* part of the
action log. The idea is that, given the same starting position, the server
could use the same code and the same input as the client, and arrive at roughly
the same conclusion, namely the position and speed at the beginning of the
movement packet.

## Reconciliation

The server now has two new player positions: the player position from the
beginning of the movement packet, and the player position calculated by
applying the action log to the last known player position. Ideally, these two
would be identical (and since this system does not affect the original player
motion packet, this shows that the action log does not add any delay or lag to
player motion). Realistically, the two will diverge more and more over time. The
server has three basic choices how to deal with this situation:

1. If the discrepancy is small, the server can just accept the client side
value. This should counteract any gradual divergence.
2. The server can also accept large discrepancies, but log this as a possible
cheat. This could mean informing an admin or incrementing a strike counter for
the player. After enough strikes in a short time, see choice 3.
3. The server can send the client a motion reset command. This is functionally
equivalent to teleporting the player; see below

## Teleporting

Sometimes the server wants to override the player's position. Examples are
teleporting, knockbacks, or motion resets as above. The server sends the client
the exact position and speed of the player. The server also remembers the last
position and speed it sent.

The client, upon receiving such a position and speed command, adjusts its own
player position and speed to exactly these values. Then it discards any action
log entries it had accumulated so far. The purpose of these entries was to
provide proof of the player's position *before* the teleport, and they are
therefore useless afterwards. The first entry of the new action log is an
acknowledgement of the new position and speed, the next entries are then actions
(control settings) from this point on.

## Disabling

The server can disable or pause the entire system by requesting action log
version 0. The client does not need to calculate or send action log items while
the version is 0. The system restarts when the server requests a greater
version. For best results, the server should also send a client position and
speed, so that the action log restarts from a known point. Alternatively, the
server could fully trust the first position given in the next position update &
action log packet it recieves.
