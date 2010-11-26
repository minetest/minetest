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

#define SERVERMAP_DELETE_UNUSED_SECTORS_TIMEOUT (60*10)
#define SERVER_MAP_SAVE_INTERVAL (60)

//#define SERVERMAP_DELETE_UNUSED_SECTORS_TIMEOUT (5)
//#define SERVER_MAP_SAVE_INTERVAL (5)

#define FOV_ANGLE (PI/2.5)

// The absolute working limit is (2^15 - viewing_range).
#define MAP_GENERATION_LIMIT (31000)

//#define MAX_SIMULTANEOUS_BLOCK_SENDS 7
//#define MAX_SIMULTANEOUS_BLOCK_SENDS 3
#define MAX_SIMULTANEOUS_BLOCK_SENDS 2
//#define MAX_SIMULTANEOUS_BLOCK_SENDS 1

#define FULL_BLOCK_SEND_ENABLE_MIN_TIME_FROM_BUILDING 2.0
#define LIMITED_MAX_SIMULTANEOUS_BLOCK_SENDS 1

// Viewing range stuff

#define FPS_DEFAULT_WANTED 30
#define FPS_DEFAULT_MAX 60

#define FORCEDFETCH_RANGE 80

#define HEIGHTMAP_RANGE_NODES 300

// The freetime ratio is dynamically kept high enough to make this
// dtime jitter possible
// Allow 50% = 0.1
/*#define DTIME_JITTER_MAX_FRACTION 0.5
#define FREETIME_RATIO_MIN 0.05
#define FREETIME_RATIO_MAX 0.4*/

//#define FREETIME_RATIO 0.2
#define FREETIME_RATIO 0.15

#define SECTOR_HEIGHTMAP_SPLIT 2

#define PLAYER_INVENTORY_SIZE (8*4)

#define SIGN_TEXT_MAX_LENGTH 50

#define ACTIVE_OBJECT_D_BLOCKS 2

#define CATCH_UNJANDLED_EXCEPTIONS 1

#endif

