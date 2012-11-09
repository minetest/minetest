/*
Minetest-c55
Copyright (C) 2012 celeron55, Perttu Ahola <celeron55@gmail.com>

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

#include "rollback.h"
#include <fstream>
#include <list>
#include <sstream>
#include "log.h"
#include "mapnode.h"
#include "gamedef.h"
#include "nodedef.h"
#include "util/serialize.h"
#include "util/string.h"
#include "strfnd.h"
#include "util/numeric.h"
#include "inventorymanager.h" // deserializing InventoryLocations

#define PP(x) "("<<(x).X<<","<<(x).Y<<","<<(x).Z<<")"

#define POINTS_PER_NODE (16.0)

// Get nearness factor for subject's action for this action
// Return value: 0 = impossible, >0 = factor
static float getSuspectNearness(bool is_guess, v3s16 suspect_p, int suspect_t,
		v3s16 action_p, int action_t)
{
	// Suspect cannot cause things in the past
	if(action_t < suspect_t)
		return 0; // 0 = cannot be
	// Start from 100
	int f = 100;
	// Distance (1 node = -x points)
	f -= POINTS_PER_NODE * intToFloat(suspect_p, 1).getDistanceFrom(intToFloat(action_p, 1));
	// Time (1 second = -x points)
	f -= 1 * (action_t - suspect_t);
	// If is a guess, halve the points
	if(is_guess)
		f *= 0.5;
	// Limit to 0
	if(f < 0)
		f = 0;
	return f;
}

class RollbackManager: public IRollbackManager
{
public:
	// IRollbackManager interface

	void reportAction(const RollbackAction &action_)
	{
		// Ignore if not important
		if(!action_.isImportant(m_gamedef))
			return;
		RollbackAction action = action_;
		action.unix_time = time(0);
		// Figure out actor
		action.actor = m_current_actor;
		action.actor_is_guess = m_current_actor_is_guess;
		// If actor is not known, find out suspect or cancel
		if(action.actor.empty()){
			v3s16 p;
			if(!action.getPosition(&p))
				return;
			action.actor = getSuspect(p, 83, 1);
			if(action.actor.empty())
				return;
			action.actor_is_guess = true;
		}
		infostream<<"RollbackManager::reportAction():"
				<<" time="<<action.unix_time
				<<" actor=\""<<action.actor<<"\""
				<<(action.actor_is_guess?" (guess)":"")
				<<" action="<<action.toString()
				<<std::endl;
		addAction(action);
	}
	std::string getActor()
	{
		return m_current_actor;
	}
	bool isActorGuess()
	{
		return m_current_actor_is_guess;
	}
	void setActor(const std::string &actor, bool is_guess)
	{
		m_current_actor = actor;
		m_current_actor_is_guess = is_guess;
	}
	std::string getSuspect(v3s16 p, float nearness_shortcut, float min_nearness)
	{
		if(m_current_actor != "")
			return m_current_actor;
		int cur_time = time(0);
		int first_time = cur_time - (100-min_nearness);
		RollbackAction likely_suspect;
		float likely_suspect_nearness = 0;
		for(std::list<RollbackAction>::const_reverse_iterator
				i = m_action_latest_buffer.rbegin();
				i != m_action_latest_buffer.rend(); i++)
		{
			if(i->unix_time < first_time)
				break;
			if(i->actor == "")
				continue;
			// Find position of suspect or continue
			v3s16 suspect_p;
			if(!i->getPosition(&suspect_p))
				continue;
			float f = getSuspectNearness(i->actor_is_guess, suspect_p,
					i->unix_time, p, cur_time);
			if(f >= min_nearness && f > likely_suspect_nearness){
				likely_suspect_nearness = f;
				likely_suspect = *i;
				if(likely_suspect_nearness >= nearness_shortcut)
					break;
			}
		}
		// No likely suspect was found
		if(likely_suspect_nearness == 0)
			return "";
		// Likely suspect was found
		return likely_suspect.actor;
	}
	void flush()
	{
		infostream<<"RollbackManager::flush()"<<std::endl;
		std::ofstream of(m_filepath.c_str(), std::ios::app);
		if(!of.good()){
			errorstream<<"RollbackManager::flush(): Could not open file "
					<<"for appending: \""<<m_filepath<<"\""<<std::endl;
			return;
		}
		for(std::list<RollbackAction>::const_iterator
				i = m_action_todisk_buffer.begin();
				i != m_action_todisk_buffer.end(); i++)
		{
			// Do not save stuff that does not have an actor
			if(i->actor == "")
				continue;
			of<<i->unix_time;
			of<<" ";
			of<<serializeJsonString(i->actor);
			of<<" ";
			of<<i->toString();
			if(i->actor_is_guess){
				of<<" ";
				of<<"actor_is_guess";
			}
			of<<std::endl;
		}
		m_action_todisk_buffer.clear();
	}
	
	// Other

	RollbackManager(const std::string &filepath, IGameDef *gamedef):
		m_filepath(filepath),
		m_gamedef(gamedef),
		m_current_actor_is_guess(false)
	{
		infostream<<"RollbackManager::RollbackManager("<<filepath<<")"
				<<std::endl;
	}
	~RollbackManager()
	{
		infostream<<"RollbackManager::~RollbackManager()"<<std::endl;
		flush();
	}

	void addAction(const RollbackAction &action)
	{
		m_action_todisk_buffer.push_back(action);
		m_action_latest_buffer.push_back(action);

		// Flush to disk sometimes
		if(m_action_todisk_buffer.size() >= 100)
			flush();
	}
	
	bool readFile(std::list<RollbackAction> &dst)
	{
		// Load whole file to memory
		std::ifstream f(m_filepath.c_str(), std::ios::in);
		if(!f.good()){
			errorstream<<"RollbackManager::readFile(): Could not open "
					<<"file for reading: \""<<m_filepath<<"\""<<std::endl;
			return false;
		}
		for(;;){
			if(f.eof() || !f.good())
				break;
			std::string line;
			std::getline(f, line);
			line = trim(line);
			if(line == "")
				continue;
			std::istringstream is(line);
			
			try{
				std::string action_time_raw;
				std::getline(is, action_time_raw, ' ');
				std::string action_actor;
				try{
					action_actor = deSerializeJsonString(is);
				}catch(SerializationError &e){
					errorstream<<"RollbackManager: Error deserializing actor: "
							<<e.what()<<std::endl;
					throw e;
				}
				RollbackAction action;
				action.unix_time = stoi(action_time_raw);
				action.actor = action_actor;
				int c = is.get();
				if(c != ' '){
					is.putback(c);
					throw SerializationError("readFile(): second ' ' not found");
				}
				action.fromStream(is);
				/*infostream<<"RollbackManager::readFile(): Action from disk: "
						<<action.toString()<<std::endl;*/
				dst.push_back(action);
			}
			catch(SerializationError &e){
				errorstream<<"RollbackManager: Error on line: "<<line<<std::endl;
				errorstream<<"RollbackManager: ^ error: "<<e.what()<<std::endl;
			}
		}
		return true;
	}

	std::list<RollbackAction> getEntriesSince(int first_time)
	{
		infostream<<"RollbackManager::getEntriesSince("<<first_time<<")"<<std::endl;
		// Collect enough data to this buffer
		std::list<RollbackAction> action_buffer;
		// Use the latest buffer if it is long enough
		if(!m_action_latest_buffer.empty() &&
				m_action_latest_buffer.begin()->unix_time <= first_time){
			action_buffer = m_action_latest_buffer;
		}
		else
		{
			// Save all remaining stuff
			flush();
			// Load whole file to memory
			bool good = readFile(action_buffer);
			if(!good){
				errorstream<<"RollbackManager::getEntriesSince(): Failed to"
						<<" open file; using data in memory."<<std::endl;
				action_buffer = m_action_latest_buffer;
			}
		}
		return action_buffer;
	}
	
	std::string getLastNodeActor(v3s16 p, int range, int seconds,
			v3s16 *act_p, int *act_seconds)
	{
		infostream<<"RollbackManager::getLastNodeActor("<<PP(p)
				<<", "<<seconds<<")"<<std::endl;
		// Figure out time
		int cur_time = time(0);
		int first_time = cur_time - seconds;

		std::list<RollbackAction> action_buffer = getEntriesSince(first_time);
		
		std::list<RollbackAction> result;

		for(std::list<RollbackAction>::const_reverse_iterator
				i = action_buffer.rbegin();
				i != action_buffer.rend(); i++)
		{
			if(i->unix_time < first_time)
				break;

			// Find position of action or continue
			v3s16 action_p;
			if(!i->getPosition(&action_p))
				continue;

			if(range == 0){
				if(action_p != p)
					continue;
			} else {
				if(abs(action_p.X - p.X) > range ||
						abs(action_p.Y - p.Y) > range ||
						abs(action_p.Z - p.Z) > range)
					continue;
			}
			
			if(act_p)
				*act_p = action_p;
			if(act_seconds)
				*act_seconds = cur_time - i->unix_time;
			return i->actor;
		}
		return "";
	}

	std::list<RollbackAction> getRevertActions(const std::string &actor_filter,
			int seconds)
	{
		infostream<<"RollbackManager::getRevertActions("<<actor_filter
				<<", "<<seconds<<")"<<std::endl;
		// Figure out time
		int cur_time = time(0);
		int first_time = cur_time - seconds;
		
		std::list<RollbackAction> action_buffer = getEntriesSince(first_time);
		
		std::list<RollbackAction> result;

		for(std::list<RollbackAction>::const_reverse_iterator
				i = action_buffer.rbegin();
				i != action_buffer.rend(); i++)
		{
			if(i->unix_time < first_time)
				break;
			if(i->actor != actor_filter)
				continue;
			const RollbackAction &action = *i;
			/*infostream<<"RollbackManager::revertAction(): Should revert"
					<<" time="<<action.unix_time
					<<" actor=\""<<action.actor<<"\""
					<<" action="<<action.toString()
					<<std::endl;*/
			result.push_back(action);
		}

		return result;
	}

private:
	std::string m_filepath;
	IGameDef *m_gamedef;
	std::string m_current_actor;
	bool m_current_actor_is_guess;
	std::list<RollbackAction> m_action_todisk_buffer;
	std::list<RollbackAction> m_action_latest_buffer;
};

IRollbackManager *createRollbackManager(const std::string &filepath, IGameDef *gamedef)
{
	return new RollbackManager(filepath, gamedef);
}


