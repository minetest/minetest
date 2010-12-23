/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#ifndef CONSTANTS_HEADER
#define CONSTANTS_HEADER

#define DEBUGFILE "debug.txt"

// Define for simulating the quirks of sending through internet
// WARNING: This disables unit testing of socket and connection
#define INTERNET_SIMULATOR 0

#define CONNECTION_TIMEOUT 30

#define RESEND_TIMEOUT_MIN 0.333
#define RESEND_TIMEOUT_MAX 3.0
// resend_timeout = avg_rtt * this
#define RESEND_TIMEOUT_FACTOR 4

#define PI 3.14159

// This is the same as in minecraft and everything else
#define FOV_ANGLE (PI/2.5)

// The absolute working limit is (2^15 - viewing_range).
#define MAP_GENERATION_LIMIT (31000)

// Time after building, during which the following limit
// is in use
//#define FULL_BLOCK_SEND_ENABLE_MIN_TIME_FROM_BUILDING 2.0
// This many blocks are sent when player is building
#define LIMITED_MAX_SIMULTANEOUS_BLOCK_SENDS 0
// Override for the previous one when distance of block
// is very low
#define BLOCK_SEND_DISABLE_LIMITS_MAX_D 1

// Viewing range stuff

//#define FREETIME_RATIO 0.15
//#define FREETIME_RATIO 0.0
#define FREETIME_RATIO 0.05

// Sectors are split to SECTOR_HEIGHTMAP_SPLIT^2 heightmaps
#define SECTOR_HEIGHTMAP_SPLIT 2

#define PLAYER_INVENTORY_SIZE (8*4)

#define SIGN_TEXT_MAX_LENGTH 50

// Whether to catch all std::exceptions.
// Assert will be called on such an event.
#ifdef DEBUG
	#define CATCH_UNHANDLED_EXCEPTIONS 0
#else
	#define CATCH_UNHANDLED_EXCEPTIONS 1
#endif

/*
	Collecting active blocks is stopped after object data
	size reaches this
*/
#define MAX_OBJECTDATA_SIZE 450

#define WATER_LEVEL (0)

// Length of cracking animation in count of images
#define CRACK_ANIMATION_LENGTH 5

#endif

