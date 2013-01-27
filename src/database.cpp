#include "map.h"
#include "mapsector.h"
#include "mapblock.h"
#include "main.h"
#include "filesys.h"
#include "voxel.h"
#include "porting.h"
#include "mapgen.h"
#include "nodemetadata.h"
#include "settings.h"
#include "log.h"
#include "profiler.h"
#include "nodedef.h"
#include "gamedef.h"
#include "util/directiontables.h"
#include "rollback_interface.h"


#include "database.h"

static s32 unsignedToSigned(s32 i, s32 max_positive)
{
	if(i < max_positive)
		return i;
	else
		return i - 2*max_positive;
}

// modulo of a negative number does not work consistently in C
static s64 pythonmodulo(s64 i, s64 mod)
{
	if(i >= 0)
		return i % mod;
	return mod - ((-i) % mod);
}

long long Database::getBlockAsInteger(const v3s16 pos) {
	return (unsigned long long)pos.Z*16777216 +
		(unsigned long long)pos.Y*4096 + 
		(unsigned long long)pos.X;
}

v3s16 Database::getIntegerAsBlock(long long i) {
	s32 x = unsignedToSigned(pythonmodulo(i, 4096), 2048);
	i = (i - x) / 4096;
	s32 y = unsignedToSigned(pythonmodulo(i, 4096), 2048);
	i = (i - y) / 4096;
	s32 z = unsignedToSigned(pythonmodulo(i, 4096), 2048);
	return v3s16(x,y,z);
}
