/*
  Minetest
  Copyright (C) 2013 sapier, sapier at gmx dot net

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

/******************************************************************************/
/* Includes                                                                   */
/******************************************************************************/

#include "pathfinder.h"
#include "environment.h"
#include "map.h"
#include "log.h"

#ifdef PATHFINDER_DEBUG
#include <iomanip>
#endif
#ifdef PATHFINDER_CALC_TIME
#include <sys/time.h>
#endif

#include <set>

/******************************************************************************/
/* Typedefs and macros                                                        */
/******************************************************************************/

//#define PATHFINDER_CALC_TIME

/** shortcut to print a 3d pos */
#define PPOS(pos) "(" << pos.X << "," << pos.Y << "," << pos.Z << ")"

#define LVL "(" << level << ")" <<

#ifdef PATHFINDER_DEBUG
#define DEBUG_OUT(a)     dstream << a
#define INFO_TARGET      std::cout
#define VERBOSE_TARGET   std::cout
#define ERROR_TARGET     std::cout
#else
#define DEBUG_OUT(a)     ;
#define INFO_TARGET      infostream << "pathfinder: "
#define VERBOSE_TARGET   verbosestream << "pathfinder: "
#define ERROR_TARGET     errorstream << "pathfinder: "
#endif

/******************************************************************************/
/* implementation                                                             */
/******************************************************************************/

std::vector<v3s16> getPath(ServerEnvironment* env,
                           v3s16 source,
                           v3s16 destination,
                           unsigned int searchdistance,
                           unsigned int max_jump,
                           unsigned int max_drop,
                           Algorithm algo,
                           Adjacency adjacency)
{

	PathFinder searchclass;

	return searchclass.getPath(env, source, destination, searchdistance,
	                           max_jump, max_drop, algo, adjacency);
}

/******************************************************************************/
PathCost::PathCost() :
	valid(false),
	value(0),
	direction(0),
	updated(false)
{

}

/******************************************************************************/
PathCost::PathCost(const PathCost& b) :
	valid(b.valid),
	value(b.value),
	direction(b.direction),
	updated(b.updated)
{
}

/******************************************************************************/
PathCost& PathCost::operator= (const PathCost& b)
{
	valid     = b.valid;
	direction = b.direction;
	value     = b.value;
	updated   = b.updated;

	return *this;
}

/******************************************************************************/
PathGridnode::PathGridnode() :
	valid(false),
	target(false),
	source(false),
	totalcost(-1),
	sourcedir(v3s16(0,0,0)),
	pos(v3s16(0,0,0)),
	is_element(false),
	type('u')
{
	//intentionaly empty
}

/******************************************************************************/
PathGridnode::PathGridnode(const PathGridnode& b) :
	valid(b.valid),
	target(b.target),
	source(b.source),
	totalcost(b.totalcost),
	sourcedir(b.sourcedir),
	pos(b.pos),
	is_element(b.is_element),
	type(b.type)
{
	directions[DIR_XP] = b.directions[DIR_XP];
	directions[DIR_XM] = b.directions[DIR_XM];
	directions[DIR_ZP] = b.directions[DIR_ZP];
	directions[DIR_ZM] = b.directions[DIR_ZM];
}

/******************************************************************************/
PathGridnode& PathGridnode::operator= (const PathGridnode& b)
{
	valid      = b.valid;
	target     = b.target;
	source     = b.source;
	is_element = b.is_element;
	totalcost  = b.totalcost;
	sourcedir  = b.sourcedir;
	pos        = b.pos;
	type       = b.type;

	directions[DIR_XP] = b.directions[DIR_XP];
	directions[DIR_XM] = b.directions[DIR_XM];
	directions[DIR_ZP] = b.directions[DIR_ZP];
	directions[DIR_ZM] = b.directions[DIR_ZM];

	return *this;
}

/******************************************************************************/
PathCost PathGridnode::getCost(v3s16 dir)
{
	if (dir.X > 0) {
		return directions[DIR_XP];
	}
	if (dir.X < 0) {
		return directions[DIR_XM];
	}
	if (dir.Z > 0) {
		return directions[DIR_ZP];
	}
	if (dir.Z < 0) {
		return directions[DIR_ZM];
	}
	PathCost retval;
	return retval;
}

/******************************************************************************/
void PathGridnode::setCost(v3s16 dir, PathCost cost)
{
	if (dir.X > 0) {
		directions[DIR_XP] = cost;
	}
	if (dir.X < 0) {
		directions[DIR_XM] = cost;
	}
	if (dir.Z > 0) {
		directions[DIR_ZP] = cost;
	}
	if (dir.Z < 0) {
		directions[DIR_ZM] = cost;
	}
}

/******************************************************************************/
std::vector<v3s16> PathFinder::getPath(ServerEnvironment* env,
                                       v3s16 source,
                                       v3s16 destination,
                                       unsigned int searchdistance,
                                       unsigned int max_jump,
                                       unsigned int max_drop,
                                       Algorithm algo,
                                       Adjacency adjacency)
{
#ifdef PATHFINDER_CALC_TIME
	timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);
#endif
	std::vector<v3s16> retval;

	//check parameters
	if (env == 0) {
		ERROR_TARGET << "missing environment pointer" << std::endl;
		return retval;
	}

	m_searchdistance = searchdistance;
	m_env = env;
	m_maxjump = max_jump;
	m_maxdrop = max_drop;
	m_start       = source;
	m_destination = destination;
	m_min_target_distance = -1;
	m_prefetch = true;

	if (algo == A_PLAIN_NP) {
		m_prefetch = false;
	}

	int min_x = MYMIN(source.X,destination.X);
	int max_x = MYMAX(source.X,destination.X);

	int min_y = MYMIN(source.Y,destination.Y);
	int max_y = MYMAX(source.Y,destination.Y);

	int min_z = MYMIN(source.Z,destination.Z);
	int max_z = MYMAX(source.Z,destination.Z);

	m_limits.X.min = min_x - searchdistance;
	m_limits.X.max = max_x + searchdistance;
	m_limits.Y.min = min_y - searchdistance;
	m_limits.Y.max = max_y + searchdistance;
	m_limits.Z.min = min_z - searchdistance;
	m_limits.Z.max = max_z + searchdistance;

	m_max_index_x = m_limits.X.max - m_limits.X.min;
	m_max_index_y = m_limits.Y.max - m_limits.Y.min;
	m_max_index_z = m_limits.Z.max - m_limits.Z.min;

	//build data map
	if (!buildCostmap()) {
		ERROR_TARGET << "failed to build costmap" << std::endl;
		return retval;
	}
#ifdef PATHFINDER_DEBUG
	printType();
	printCost();
	printYDir();
#endif

	//validate and mark start and end pos
	v3s16 start_index  = getIndexPos(source);
	v3s16 end_index    = getIndexPos(destination);

	PathGridnode& startpos = getIndexElement(start_index);
	PathGridnode& endpos   = getIndexElement(end_index);

	if (!startpos.valid) {
		VERBOSE_TARGET << "invalid startpos" <<
			"Index: " << PPOS(start_index) <<
			"Realpos: " << PPOS(getRealPos(start_index)) << std::endl;
		return retval;
	}
	if (!endpos.valid) {
		VERBOSE_TARGET << "invalid stoppos" <<
			"Index: " << PPOS(end_index) <<
			"Realpos: " << PPOS(getRealPos(end_index)) << std::endl;
		return retval;
	}

	endpos.target      = true;
	startpos.source    = true;
	startpos.totalcost = 0;

	bool update_cost_retval = false;

	switch (algo) {
	case A_PLAIN_NP:
	case A_PLAIN:
		// update_cost_retval = update_cost_heuristic(StartIndex,v3s16(0,0,0),0,0);
		update_cost_retval = updateCostHeuristic(start_index, m_adjacency_4);
		break;
	default:
		ERROR_TARGET << "missing algorithm"<< std::endl;
		break;
	}

	if (update_cost_retval) {

#ifdef PATHFINDER_DEBUG
		std::cout << "Path to target found!" << std::endl;
		printPathlen();
#endif

		//find path
		std::vector<v3s16> path;
		buildPath(path, end_index, 0);

#ifdef PATHFINDER_DEBUG
		std::cout << "Full index path:" << std::endl;
		printPath(path);
#endif

		//optimize path
		std::vector<v3s16> optimized_path;

		std::vector<v3s16>::iterator startpos = path.begin();
		optimized_path.push_back(source);

		for (std::vector<v3s16>::iterator i = path.begin();
		     i != path.end(); ++i) {
			if (!m_env->line_of_sight(
				    tov3f(getIndexElement(*startpos).pos),
				    tov3f(getIndexElement(*i).pos))) {
				optimized_path.push_back(getIndexElement(*(i-1)).pos);
				startpos = (i-1);
			}
		}

		optimized_path.push_back(destination);

#ifdef PATHFINDER_DEBUG
		std::cout << "Optimized path:" << std::endl;
		printPath(optimized_path);
#endif
#ifdef PATHFINDER_CALC_TIME
		timespec ts2;
		clock_gettime(CLOCK_REALTIME, &ts2);

		int ms = (ts2.tv_nsec - ts.tv_nsec)/(1000*1000);
		int us = ((ts2.tv_nsec - ts.tv_nsec) - (ms*1000*1000))/1000;
		int ns = ((ts2.tv_nsec - ts.tv_nsec) - ( (ms*1000*1000) + (us*1000)));


		std::cout << "Calculating path took: " << (ts2.tv_sec - ts.tv_sec) <<
			"s " << ms << "ms " << us << "us " << ns << "ns " << std::endl;
#endif
		return optimized_path;
	}
	else {
#ifdef PATHFINDER_DEBUG
		printPathlen();
#endif
		ERROR_TARGET << "failed to update cost map"<< std::endl;
	}


	//return
	return retval;
}

/******************************************************************************/
PathFinder::PathFinder() :
	m_max_index_x(0),
	m_max_index_y(0),
	m_max_index_z(0),
	m_searchdistance(0),
	m_maxdrop(0),
	m_maxjump(0),
	m_min_target_distance(0),
	m_prefetch(true),
	m_start(0,0,0),
	m_destination(0,0,0),
	m_limits(),
	m_data(),
	m_env(0)
{
	m_adjacency_4.push_back(v3s16(-1, 0, 0));
	m_adjacency_4.push_back(v3s16(1, 0, 0));
	m_adjacency_4.push_back(v3s16(0, 1, 0));
	m_adjacency_4.push_back(v3s16(0, -1, 0));
}

/******************************************************************************/
v3s16 PathFinder::getRealPos(v3s16 ipos)
{
	return v3s16(ipos.X + m_limits.X.min,
	             ipos.Y + m_limits.Y.min,
	             ipos.Z + m_limits.Z.min);
}

/******************************************************************************/
bool PathFinder::buildCostmap()
{
	INFO_TARGET << "PathFinder build costmap: (" << m_limits.X.min << ","
	            << m_limits.Z.min << ") ("
	            << m_limits.X.max << ","
	            << m_limits.Z.max << ")"
	            << std::endl;
	m_data.resize(m_max_index_x);
	for (int x = 0; x < m_max_index_x; ++x) {
		m_data[x].resize(m_max_index_z);
		for (int z = 0; z < m_max_index_z; ++z) {
			m_data[x][z].resize(m_max_index_y);

			int surfaces = 0;
			for (int y = 0; y < m_max_index_y; ++y) {
				v3s16 ipos(x,y,z);

				v3s16 realpos = getRealPos(ipos);

				MapNode current = m_env->getMap().getNodeNoEx(realpos);
				MapNode below   = m_env->getMap().getNodeNoEx(realpos + v3s16(0, -1, 0));


				if ((current.param0 == CONTENT_IGNORE) ||
				    (below.param0 == CONTENT_IGNORE)) {
					DEBUG_OUT("PathFinder: " << PPOS(realpos) <<
					          " current or below is invalid element" << std::endl);
					if (current.param0 == CONTENT_IGNORE) {
						m_data[x][z][y].type = 'i';
						DEBUG_OUT(x << "," << y << "," << z << ": " << 'i' << std::endl);
					}
					continue;
				}

				//don't add anything if it isn't an air node
				if ((current.param0 != CONTENT_AIR) ||
				    (below.param0 == CONTENT_AIR )) {
					DEBUG_OUT("PathFinder: " << PPOS(realpos)
					          << " not on surface" << std::endl);
					if (current.param0 != CONTENT_AIR) {
						m_data[x][z][y].type = 's';
						DEBUG_OUT(x << "," << y << "," << z << ": " << 's' << std::endl);
					}
					else {
						m_data[x][z][y].type   = '-';
						DEBUG_OUT(x << "," << y << "," << z << ": " << '-' << std::endl);
					}
					continue;
				}

				++surfaces;

				m_data[x][z][y].valid  = true;
				m_data[x][z][y].pos    = realpos;
				m_data[x][z][y].type   = 'g';
				DEBUG_OUT(x << "," << y << "," << z << ": " << 'a' << std::endl);

				if (m_prefetch) {
					m_data[x][z][y].directions[DIR_XP] =
						calcCost(realpos,v3s16( 1,0, 0));
					m_data[x][z][y].directions[DIR_XM] =
						calcCost(realpos,v3s16(-1,0, 0));
					m_data[x][z][y].directions[DIR_ZP] =
						calcCost(realpos,v3s16( 0,0, 1));
					m_data[x][z][y].directions[DIR_ZM] =
						calcCost(realpos,v3s16( 0,0,-1));
				}

			}
		}
	}
	return true;
}

/******************************************************************************/
PathCost PathFinder::calcCost(v3s16 pos, v3s16 dir)
{
	PathCost retval;

	retval.updated = true;

	v3s16 pos2 = pos + dir;

	//check limits
	if (    (pos2.X < m_limits.X.min) ||
	        (pos2.X >= m_limits.X.max) ||
	        (pos2.Z < m_limits.Z.min) ||
	        (pos2.Z >= m_limits.Z.max)) {
		DEBUG_OUT("PathFinder: " << PPOS(pos2) <<
		          " no cost -> out of limits" << std::endl);
		return retval;
	}

	MapNode node_at_pos2 = m_env->getMap().getNodeNoEx(pos2);

	//did we get information about node?
	if (node_at_pos2.param0 == CONTENT_IGNORE ) {
		VERBOSE_TARGET << "PathFinder: (1) area at pos: "
		               << PPOS(pos2) << " not loaded";
		return retval;
	}

	if (node_at_pos2.param0 == CONTENT_AIR) {
		MapNode node_below_pos2 =
			m_env->getMap().getNodeNoEx(pos2 + v3s16(0,-1,0));

		//did we get information about node?
		if (node_below_pos2.param0 == CONTENT_IGNORE ) {
			VERBOSE_TARGET << "PathFinder: (2) area at pos: "
			               << PPOS((pos2 + v3s16(0,-1,0))) << " not loaded";
			return retval;
		}

		if (node_below_pos2.param0 != CONTENT_AIR) {
			retval.valid = true;
			retval.value = 1;
			retval.direction = 0;
			DEBUG_OUT("PathFinder: "<< PPOS(pos)
			          << " cost same height found" << std::endl);
		}
		else {
			v3s16 testpos = pos2 - v3s16(0,-1,0);
			MapNode node_at_pos = m_env->getMap().getNodeNoEx(testpos);

			while ((node_at_pos.param0 != CONTENT_IGNORE) &&
			       (node_at_pos.param0 == CONTENT_AIR) &&
			       (testpos.Y > m_limits.Y.min)) {
				testpos += v3s16(0,-1,0);
				node_at_pos = m_env->getMap().getNodeNoEx(testpos);
			}

			//did we find surface?
			if ((testpos.Y >= m_limits.Y.min) &&
			    (node_at_pos.param0 != CONTENT_IGNORE) &&
			    (node_at_pos.param0 != CONTENT_AIR)) {
				if (((pos2.Y - testpos.Y)*-1) <= m_maxdrop) {
					retval.valid = true;
					retval.value = 2;
					//difference of y-pos +1 (target node is ABOVE solid node)
					retval.direction = ((testpos.Y - pos2.Y) +1);
					DEBUG_OUT("PathFinder cost below height found" << std::endl);
				}
				else {
					INFO_TARGET << "PathFinder:"
						" distance to surface below to big: "
					            << (testpos.Y - pos2.Y) << " max: " << m_maxdrop
					            << std::endl;
				}
			}
			else {
				DEBUG_OUT("PathFinder: no surface below found" << std::endl);
			}
		}
	}
	else {
		v3s16 testpos = pos2;
		MapNode node_at_pos = m_env->getMap().getNodeNoEx(testpos);

		while ((node_at_pos.param0 != CONTENT_IGNORE) &&
		       (node_at_pos.param0 != CONTENT_AIR) &&
		       (testpos.Y < m_limits.Y.max)) {
			testpos += v3s16(0,1,0);
			node_at_pos = m_env->getMap().getNodeNoEx(testpos);
		}

		//did we find surface?
		if ((testpos.Y <= m_limits.Y.max) &&
		    (node_at_pos.param0 == CONTENT_AIR)) {

			if (testpos.Y - pos2.Y <= m_maxjump) {
				retval.valid = true;
				retval.value = 2;
				retval.direction = (testpos.Y - pos2.Y);
				DEBUG_OUT("PathFinder cost above found" << std::endl);
			}
			else {
				DEBUG_OUT("PathFinder: distance to surface above to big: "
				          << (testpos.Y - pos2.Y) << " max: " << m_maxjump
				          << std::endl);
			}
		}
		else {
			DEBUG_OUT("PathFinder: no surface above found" << std::endl);
		}
	}
	return retval;
}

/******************************************************************************/
v3s16 PathFinder::getIndexPos(v3s16 pos)
{
	return v3s16(pos.X - m_limits.X.min,
	             pos.Y - m_limits.Y.min,
	             pos.Z - m_limits.Z.min);
}

/******************************************************************************/
PathGridnode& PathFinder::getIndexElement(v3s16 ipos)
{
	return m_data[ipos.X][ipos.Z][ipos.Y];
}

/******************************************************************************/
bool PathFinder::validIndex(v3s16 index)
{
	if ((index.X < m_max_index_x) &&
		(index.Y < m_max_index_y) &&
		(index.Z < m_max_index_z) &&
		(index.X >= 0) &&
		(index.Y >= 0) &&
	    (index.Z >= 0)) {
		return true;
	}
	
	return false;
}

/******************************************************************************/
v3s16 PathFinder::invert(v3s16 pos)
{
	return v3s16(-pos.X, -pos.Y, -pos.Z);
}

bool PathFinder::updateCostHeuristic(v3s16 pos, std::vector <v3s16>& directions)
{
	std::set <std::pair <unsigned int, v3s16> > q;
	q.insert(std::make_pair(getManhattanDistance(pos, m_destination), pos));
	while(!q.empty()) {
		v3s16 current_pos = q.begin()->second;
		q.erase(q.begin());
		for(unsigned int i = 0; i < directions.size(); ++i) {
			v3s16 next_pos = current_pos + directions[i];
		}
	}
	return true;
}

/******************************************************************************/
v3s16 PathFinder::getDirHeuristic(std::vector<v3s16>& directions, PathGridnode& g_pos)
{
	int   minscore = -1;
	v3s16 retdir   = v3s16(0,0,0);
	v3s16 srcpos = g_pos.pos;
	DEBUG_OUT("PathFinder: remaining dirs at beginning:"
	          << directions.size() << std::endl);

	for (std::vector<v3s16>::iterator iter = directions.begin();
	     iter != directions.end();
	     ++iter) {

		v3s16 pos1 =  v3s16(srcpos.X + iter->X,0,srcpos.Z+iter->Z);

		int cur_manhattan = getManhattanDistance(pos1, m_destination);
		PathCost cost    = g_pos.getCost(*iter);

		if (!cost.updated) {
			cost = calcCost(g_pos.pos,*iter);
			g_pos.setCost(*iter,cost);
		}

		if (cost.valid) {
			int score = cost.value + cur_manhattan;

			if ((minscore < 0)|| (score < minscore)) {
				minscore = score;
				retdir = *iter;
			}
		}
	}

	if (retdir != v3s16(0,0,0)) {
		for (std::vector<v3s16>::iterator iter = directions.begin();
		     iter != directions.end();
		     ++iter) {
			if(*iter == retdir) {
				DEBUG_OUT("PathFinder: removing return direction" << std::endl);
				directions.erase(iter);
				break;
			}
		}
	}
	else {
		DEBUG_OUT("PathFinder: didn't find any valid direction clearing"
		          << std::endl);
		directions.clear();
	}
	DEBUG_OUT("PathFinder: remaining dirs at end:" << directions.size()
	          << std::endl);
	return retdir;
}

/******************************************************************************/
bool PathFinder::updateCostHeuristic(v3s16 ipos,
                                     v3s16 srcdir,
                                     int current_cost,
                                     int level)
{
	PathGridnode& g_pos = getIndexElement(ipos);
	g_pos.totalcost = current_cost;
	g_pos.sourcedir = srcdir;

	++level;

	//check if target has been found
	if (g_pos.target) {
		m_min_target_distance = current_cost;
		DEBUG_OUT(LVL " PathFinder: target found!" << std::endl);
		return true;
	}

	bool retval = false;

	std::vector<v3s16> directions;

	directions.push_back(v3s16( 1,0, 0));
	directions.push_back(v3s16(-1,0, 0));
	directions.push_back(v3s16( 0,0, 1));
	directions.push_back(v3s16( 0,0,-1));

	v3s16 direction = getDirHeuristic(directions,g_pos);

	while (direction != v3s16(0,0,0) && (!retval)) {

		if (direction != srcdir) {
			PathCost cost = g_pos.getCost(direction);

			if (cost.valid) {
				direction.Y = cost.direction;

				v3s16 ipos2 = ipos + direction;

				if (!validIndex(ipos2)) {
					DEBUG_OUT(LVL " PathFinder: " << PPOS(ipos2) <<
					          " out of range (" << m_limits.X.max << "," <<
					          m_limits.Y.max << "," << m_limits.Z.max
					          <<")" << std::endl);
					direction = getDirHeuristic(directions,g_pos);
					continue;
				}

				PathGridnode& g_pos2 = getIndexElement(ipos2);

				if (!g_pos2.valid) {
					VERBOSE_TARGET << LVL "PathFinder: no data for new position: "
					               << PPOS(ipos2) << std::endl;
					direction = getDirHeuristic(directions,g_pos);
					continue;
				}

				assert(cost.value > 0);

				int new_cost = current_cost + cost.value;

				// check if there already is a smaller path
				if ((m_min_target_distance > 0) &&
				    (m_min_target_distance < new_cost)) {
					DEBUG_OUT(LVL "PathFinder:"
					          " already longer than best already found path "
					          << PPOS(ipos2) << std::endl);
					return false;
				}

				if ((g_pos2.totalcost < 0) ||
				    (g_pos2.totalcost > new_cost)) {
					DEBUG_OUT(LVL "PathFinder: updating path at: "<<
					          PPOS(ipos2) << " from: " << g_pos2.totalcost << " to "<<
					          new_cost << " srcdir=" <<
					          PPOS(invert(direction))<< std::endl);
					if (updateCostHeuristic(ipos2,invert(direction),
					                        new_cost,level)) {
						retval = true;
					}
				}
				else {
					DEBUG_OUT(LVL "PathFinder:"
					          " already found shorter path to: "
					          << PPOS(ipos2) << std::endl);
				}
			}
			else {
				DEBUG_OUT(LVL "PathFinder:"
				          " not moving to invalid direction: "
				          << PPOS(direction) << std::endl);
			}
		}
		else {
			DEBUG_OUT(LVL "PathFinder:"
			          " skipping srcdir: "
			          << PPOS(direction) << std::endl);
		}
		direction = getDirHeuristic(directions,g_pos);
	}
	return retval;
}

/******************************************************************************/
void PathFinder::buildPath(std::vector<v3s16>& path, v3s16 pos, int level)
{
	++level;
	if (level > 700) {
		ERROR_TARGET
			<< LVL "PathFinder: path is too long aborting" << std::endl;
		return;
	}

	PathGridnode& g_pos = getIndexElement(pos);
	if (!g_pos.valid) {
		ERROR_TARGET
			<< LVL "PathFinder: invalid next pos detected aborting" << std::endl;
		return;
	}

	g_pos.is_element = true;

	//check if source reached
	if (g_pos.source) {
		path.push_back(pos);
		return;
	}

	buildPath(path,pos + g_pos.sourcedir,level);
	path.push_back(pos);
}

/******************************************************************************/
v3f PathFinder::tov3f(v3s16 pos)
{
	return v3f(BS*pos.X,BS*pos.Y,BS*pos.Z);
}

#ifdef PATHFINDER_DEBUG

/******************************************************************************/
void PathFinder::printCost()
{
	printCost(DIR_XP);
	printCost(DIR_XM);
	printCost(DIR_ZP);
	printCost(DIR_ZM);
}

/******************************************************************************/
void PathFinder::printYDir()
{
	printYDir(DIR_XP);
	printYDir(DIR_XM);
	printYDir(DIR_ZP);
	printYDir(DIR_ZM);
}

/******************************************************************************/
void PathFinder::printCost(PathDirections dir)
{

	DEBUG_OUT("Cost in direction: " << dirToName(dir) << std::endl);
	DEBUG_OUT(std::setfill('-') << std::setw(80) << "-" << std::endl);
	DEBUG_OUT(std::setfill(' '));
	for (int y = 0; y < m_max_index_y; ++y) {

		DEBUG_OUT("Level: " << y << std::endl);

		DEBUG_OUT(std::setw(4) << " " << "  ");
		for (int x = 0; x < m_max_index_x; ++x) {
			DEBUG_OUT(std::setw(4) << x);
		}
		DEBUG_OUT(std::endl);

		for (int z = 0; z < m_max_index_z; ++z) {
			DEBUG_OUT(std::setw(4) << z <<": ");
			for (int x = 0; x < m_max_index_x; ++x) {
				if (m_data[x][z][y].directions[dir].valid) {
					DEBUG_OUT(std::setw(4)
					          << m_data[x][z][y].directions[dir].value);
				} else {
					DEBUG_OUT(std::setw(4) << "-");
				}
			}
			DEBUG_OUT(std::endl);
		}
		DEBUG_OUT(std::endl);
	}
}

/******************************************************************************/
void PathFinder::printYDir(PathDirections dir)
{
	std::cout << "Height difference in direction: " << dirToName(dir) << std::endl;
	std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
	std::cout << std::setfill(' ');
	for (int y = 0; y < m_max_index_y; ++y) {

		std::cout << "Level: " << y << std::endl;

		std::cout << std::setw(4) << " " << "  ";
		for (int x = 0; x < m_max_index_x; ++x) {
			std::cout << std::setw(4) << x;
		}
		std::cout << std::endl;

		for (int z = 0; z < m_max_index_z; ++z) {
			std::cout << std::setw(4) << z <<": ";
			for (int x = 0; x < m_max_index_x; ++x) {
				if (m_data[x][z][y].directions[dir].valid)
					std::cout << std::setw(4)
					          << m_data[x][z][y].directions[dir].direction;
				else
					std::cout << std::setw(4) << "-";
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
}

/******************************************************************************/
void PathFinder::printType()
{
	std::cout << "Type of node:" << std::endl;
	std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
	std::cout << std::setfill(' ');
	for (int y = 0; y < m_max_index_y; ++y) {

		std::cout << "Level: " << y << std::endl;

		std::cout << std::setw(3) << " " << "  ";
		for (int x = 0; x < m_max_index_x; ++x) {
			std::cout << std::setw(3) << x;
		}
		std::cout << std::endl;

		for (int z = 0; z < m_max_index_z; ++z) {
			std::cout << std::setw(3) << z <<": ";
			for (int x = 0; x < m_max_index_x; ++x) {
				char toshow = m_data[x][z][y].type;
				std::cout << std::setw(3) << toshow;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

/******************************************************************************/
void PathFinder::printPathlen()
{
	std::cout << "Pathlen:" << std::endl;
	std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
	std::cout << std::setfill(' ');
	for (int y = 0; y < m_max_index_y; ++y) {

		std::cout << "Level: " << y << std::endl;

		std::cout << std::setw(3) << " " << "  ";
		for (int x = 0; x < m_max_index_x; ++x) {
			std::cout << std::setw(3) << x;
		}
		std::cout << std::endl;

		for (int z = 0; z < m_max_index_z; ++z) {
			std::cout << std::setw(3) << z <<": ";
			for (int x = 0; x < m_max_index_x; ++x) {
				std::cout << std::setw(3) << m_data[x][z][y].totalcost;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
	std::cout << std::endl;
}

/******************************************************************************/
std::string PathFinder::dirToName(PathDirections dir)
{
	switch (dir) {
	case DIR_XP:
		return "XP";
		break;
	case DIR_XM:
		return "XM";
		break;
	case DIR_ZP:
		return "ZP";
		break;
	case DIR_ZM:
		return "ZM";
		break;
	default:
		return "UKN";
	}
}

/******************************************************************************/
void PathFinder::printPath(std::vector<v3s16> path)
{
	unsigned int current = 0;
	for (std::vector<v3s16>::iterator i = path.begin();
	     i != path.end(); ++i) {
		std::cout << std::setw(3) << current << ":" << PPOS((*i)) << std::endl;
		current++;
	}
}

#endif
