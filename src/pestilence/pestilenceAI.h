/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#ifndef PESTILANCE_AI_HEADER
#define PESTILANCE_AI_HEADER

#include "aiplayer.h" // BS

class Map;
class IGameDef;

struct PestAIDef
{
	u16		mMaxLevel;
	u16		mMaxSize;

	PestAIDef() {
		mMaxLevel = 5;
		mMaxSize = 5;
	}
};

class PestilenceAI : AIPlayer
{
public:

	PestilenceAI(IGameDef *gamedef, const PestAIDef& aiplayerdef);
	virtual ~PestilenceAI();

	/*
		serialize() writes a bunch of text that can contain
		any characters except a '\0', and such an ending that
		deSerialize stops reading exactly at the right point.
	*/
	virtual void serialize(std::ostream &os);
	virtual void deSerialize(std::istream &is, std::string playername);

	virtual void step(f32 dtime);

protected:
	IGameDef*	mGameDef;
	PestAIDef	mPestDef;
};

#endif

