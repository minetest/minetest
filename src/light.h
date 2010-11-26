#ifndef LIGHT_HEADER
#define LIGHT_HEADER

#include "common_irrlicht.h"

// This directly sets the range of light
#define LIGHT_MAX 14
// This brightness is reserved for sunlight
#define LIGHT_SUN 15

inline u8 diminish_light(u8 light)
{
	if(light == 0)
		return 0;
	if(light >= LIGHT_MAX)
		return LIGHT_MAX - 1;
		
	return light - 1;
}

inline u8 diminish_light(u8 light, u8 distance)
{
	if(distance >= light)
		return 0;
	return  light - distance;
}

inline u8 undiminish_light(u8 light)
{
	// We don't know if light should undiminish from this particular 0.
	// Thus, keep it at 0.
	if(light == 0)
		return 0;
	if(light == LIGHT_MAX)
		return light;
	
	return light + 1;
}

extern u8 light_decode_table[LIGHT_MAX+1];

inline u8 decode_light(u8 light)
{
	if(light == LIGHT_SUN)
		return light_decode_table[LIGHT_MAX];
	
	if(light > LIGHT_MAX)
		throw;
	
	return light_decode_table[light];
}

#endif

