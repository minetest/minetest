/*
Minetest
Copyright (C) 2013-2014 johnDC, Jan Pidych <pidych@gmail.com>

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

#include "constants.h"
#include "gamedef.h"
#include "settings.h"
#include "content_sao.h"
#include "util/numeric.h"
#include "pestilenceAI.h"

PestilenceAI::PestilenceAI(IGameDef *gamedef, const PestAIDef& pestAIDef ):
	mGameDef(gamedef),
	mPestDef(pestAIDef)
{
}

PestilenceAI::~PestilenceAI()
{
}

void PestilenceAI::step(f32 dtime)
{

}

void PestilenceAI::serialize(std::ostream &os)
{
	// Utilize a Settings object for storing values
	Settings args;
	args.setS32("version", 1);
	args.set("name", m_name);
	args.writeLines(os);

	os<<"PlayerArgsEnd\n";
}

void PestilenceAI::deSerialize(std::istream &is, std::string playername)
{
	Settings args;
	
	for(;;)
	{
		if(is.eof())
			throw SerializationError
					(("Player::deSerialize(): PlayerArgsEnd of player \"" + playername + "\" not found").c_str());
		std::string line;
		std::getline(is, line);
		std::string trimmedline = trim(line);
		if(trimmedline == "PlayerArgsEnd")
			break;
		args.parseConfigLine(line);
	}

	//args.getS32("version"); // Version field value not used
	std::string name = args.get("name");
	
}