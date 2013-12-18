/*
Minetest
Copyright (C) 2013-2013 johnDC, Jan Pidych <pidych@gmail.com>

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

#ifndef AI_PLAYER_HEADER
#define AI_PLAYER_HEADER

#include "constants.h" // BS

#define AI_PLAYER_NOT_ACTIVE	0
#define AI_PLAYER_ACTIVE		1

#define AI_PLAYERNAME_SIZE 16
#define AI_PLAYERNAME_ALLOWED_CHARS "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_"

class Map;
class IGameDef;

class AIPlayer
{
protected:
	//Constructor is protected, you must derive own class for AI implementation
	AIPlayer(void) {};

public:

	virtual ~AIPlayer() {};

	/* Do we need this functions???
	virtual bool isLocal() const
	{ return false; }
	virtual PlayerSAO *getPlayerSAO()
	{ return NULL; }
	virtual void setPlayerSAO(PlayerSAO *sao)
	{ assert(0); }
	*/

	/*
		serialize() writes a bunch of text that can contain
		any characters except a '\0', and such an ending that
		deSerialize stops reading exactly at the right point.
	*/
	virtual void serialize(std::ostream &os) = 0;
	virtual void deSerialize(std::istream &is, std::string playername) = 0;

	u16 getActiveStatus() { return m_status; }

	virtual void step(f32 dtime) = 0;
	
protected:

	char m_name[AI_PLAYERNAME_SIZE];
	u32  m_aliance;
	u16	 m_status;  
};

#endif

