/*
Minetest
Copyright (C) 2013 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation; either version 2.1 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma once

/*
	All kinds of constants.

	Cross-platform compatibility stuff should go in porting.h.

    Some things here are legacy.
*/

/*
    Connection
*/

#define PEER_ID_INEXISTENT 0
#define PEER_ID_SERVER 1

// Define for simulating the quirks of sending through internet.
// Causes the socket class to deliberately drop random packets.
// This disables unit testing of socket and connection.
#define INTERNET_SIMULATOR 0
#define INTERNET_SIMULATOR_PACKET_LOSS 10 // 10 = easy, 4 = hard

#define CONNECTION_TIMEOUT 30

#define RESEND_TIMEOUT_MIN 0.1
#define RESEND_TIMEOUT_MAX 3.0
// resend_timeout = avg_rtt * this
#define RESEND_TIMEOUT_FACTOR 4

/*
    Server
*/

// This many blocks are sent when player is building
#define LIMITED_MAX_SIMULTANEOUS_BLOCK_SENDS 0
// Override for the previous one when distance of block is very low
#define BLOCK_SEND_DISABLE_LIMITS_MAX_D 1

/*
    Map-related things
*/

// The absolute working limit is (2^15 - viewing_range).
// I really don't want to make every algorithm to check if it's going near
// the limit or not, so this is lower.
// This is the maximum value the setting map_generation_limit can be
#define MAX_MAP_GENERATION_LIMIT (31000)

// Size of node in floating-point units
// The original idea behind this is to disallow plain casts between
// floating-point and integer positions, which potentially give wrong
// results. (negative coordinates, values between nodes, ...)
// Use floatToInt(p, BS) and intToFloat(p, BS).
#define BS 10.0f

// Dimension of a MapBlock
#define MAP_BLOCKSIZE 16
// This makes mesh updates too slow, as many meshes are updated during
// the main loop (related to TempMods and day/night)
//#define MAP_BLOCKSIZE 32

// Player step height in nodes
#define PLAYER_DEFAULT_STEPHEIGHT 0.6f

/*
    Old stuff that shouldn't be hardcoded
*/

// Size of player's main inventory
#define PLAYER_INVENTORY_SIZE (8 * 4)

// Default maximum health points of a player
#define PLAYER_MAX_HP_DEFAULT 20

// Default maximal breath of a player
#define PLAYER_MAX_BREATH_DEFAULT 10

// Number of different files to try to save a player to if the first fails
// (because of a case-insensitive filesystem)
// TODO: Use case-insensitive player names instead of this hack.
#define PLAYER_FILE_ALTERNATE_TRIES 1000

// For screenshots a serial number is appended to the filename + datetimestamp
// if filename + datetimestamp is not unique.
// This is the maximum number of attempts to try and add a serial to the end of
// the file attempting to ensure a unique filename
#define SCREENSHOT_MAX_SERIAL_TRIES 1000

/*
    GUI related things
*/

#define TTF_DEFAULT_FONT_SIZE (16)
