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

/******************************************************************************/
/* Typedefs and macros                                                        */
/******************************************************************************/

//#define PATHFINDER_CALC_TIME

/** shortcut to print a 3d pos */
#define PPOS(pos) "(" << pos.X << "," << pos.Y << "," << pos.Z << ")"

#define LVL "(" << level << ")" <<

#ifdef PATHFINDER_DEBUG
#define DEBUG_OUT(a)     std::cout << a
#define INFO_TARGET      std::cout
#define VERBOSE_TARGET   std::cout
#define ERROR_TARGET     std::cout
#else
#define DEBUG_OUT(a)     while(0)
#define INFO_TARGET      infostream << "pathfinder: "
#define VERBOSE_TARGET   verbosestream << "pathfinder: "
#define ERROR_TARGET     errorstream << "pathfinder: "
#endif

/******************************************************************************/
/* implementation                                                             */
/******************************************************************************/

std::vector<v3s16> get_Path(ServerEnvironment* env,
							v3s16 source,
							v3s16 destination,
							unsigned int searchdistance,
							unsigned int max_jump,
							unsigned int max_drop,
							algorithm algo) {

	pathfinder searchclass;

	return searchclass.get_Path(env,
				source,destination,
				searchdistance,max_jump,max_drop,algo);
}

/******************************************************************************/
path_cost::path_cost()
:	valid(false),
	value(0),
	direction(0),
	updated(false)
{
	//intentionaly empty
}

/******************************************************************************/
path_cost::path_cost(const path_cost& b) {
	valid     = b.valid;
	direction = b.direction;
	value     = b.value;
	updated   = b.updated;
}

/******************************************************************************/
path_cost& path_cost::operator= (const path_cost& b) {
	valid     = b.valid;
	direction = b.direction;
	value     = b.value;
	updated   = b.updated;

	return *this;
}

/******************************************************************************/
path_gridnode::path_gridnode()
:	valid(false),
	target(false),
	source(false),
	totalcost(-1),
	sourcedir(v3s16(0,0,0)),
	surfaces(0),
	pos(v3s16(0,0,0)),
	is_element(false),
	type('u')
{
	//intentionaly empty
}

/******************************************************************************/
path_gridnode::path_gridnode(const path_gridnode& b)
:	valid(b.valid),
	target(b.target),
	source(b.source),
	totalcost(b.totalcost),
	sourcedir(b.sourcedir),
	surfaces(b.surfaces),
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
path_gridnode& path_gridnode::operator= (const path_gridnode& b) {
	valid      = b.valid;
	target     = b.target;
	source     = b.source;
	is_element = b.is_element;
	totalcost  = b.totalcost;
	sourcedir  = b.sourcedir;
	surfaces   = b.surfaces;
	pos        = b.pos;
	type       = b.type;

	directions[DIR_XP] = b.directions[DIR_XP];
	directions[DIR_XM] = b.directions[DIR_XM];
	directions[DIR_ZP] = b.directions[DIR_ZP];
	directions[DIR_ZM] = b.directions[DIR_ZM];

	return *this;
}

/******************************************************************************/
path_cost path_gridnode::get_cost(v3s16 dir) {
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
	path_cost retval;
	return retval;
}

/******************************************************************************/
void path_gridnode::set_cost(v3s16 dir,path_cost cost) {
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
std::vector<v3s16> pathfinder::get_Path(ServerEnvironment* env,
							v3s16 source,
							v3s16 destination,
							unsigned int searchdistance,
							unsigned int max_jump,
							unsigned int max_drop,
							algorithm algo) {
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
	if (!build_costmap()) {
		ERROR_TARGET << "failed to build costmap" << std::endl;
		return retval;
	}
#ifdef PATHFINDER_DEBUG
	print_type();
	print_cost();
	print_ydir();
#endif

	//validate and mark start and end pos
	v3s16 StartIndex  = getIndexPos(source);
	v3s16 EndIndex    = getIndexPos(destination);

	path_gridnode& startpos = getIndexElement(StartIndex);
	path_gridnode& endpos   = getIndexElement(EndIndex);

	if (!startpos.valid) {
		VERBOSE_TARGET << "invalid startpos" <<
				"Index: " << PPOS(StartIndex) <<
				"Realpos: " << PPOS(getRealPos(StartIndex)) << std::endl;
		return retval;
	}
	if (!endpos.valid) {
		VERBOSE_TARGET << "invalid stoppos" <<
				"Index: " << PPOS(EndIndex) <<
				"Realpos: " << PPOS(getRealPos(EndIndex)) << std::endl;
		return retval;
	}

	endpos.target      = true;
	startpos.source    = true;
	startpos.totalcost = 0;

	bool update_cost_retval = false;

	switch (algo) {
		case DIJKSTRA:
			update_cost_retval = update_all_costs(StartIndex,v3s16(0,0,0),0,0);
			break;
		case A_PLAIN_NP:
		case A_PLAIN:
			update_cost_retval = update_cost_heuristic(StartIndex,v3s16(0,0,0),0,0);
			break;
		default:
			ERROR_TARGET << "missing algorithm"<< std::endl;
			break;
	}

	if (update_cost_retval) {

#ifdef PATHFINDER_DEBUG
		std::cout << "Path to target found!" << std::endl;
		print_pathlen();
#endif

		//find path
		std::vector<v3s16> path;
		build_path(path,EndIndex,0);

#ifdef PATHFINDER_DEBUG
		std::cout << "Full index path:" << std::endl;
		print_path(path);
#endif

		//optimize path
		std::vector<v3s16> optimized_path;

		std::vector<v3s16>::iterator startpos = path.begin();
		optimized_path.push_back(source);

		for (std::vector<v3s16>::iterator i = path.begin();
					i != path.end(); i++) {
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
		print_path(optimized_path);
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
		print_pathlen();
#endif
		ERROR_TARGET << "failed to update cost map"<< std::endl;
	}


	//return
	return retval;
}

/******************************************************************************/
pathfinder::pathfinder() :
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
	//intentionaly empty
}

/******************************************************************************/
v3s16 pathfinder::getRealPos(v3s16 ipos) {

	v3s16 retval = ipos;

	retval.X += m_limits.X.min;
	retval.Y += m_limits.Y.min;
	retval.Z += m_limits.Z.min;

	return retval;
}

/******************************************************************************/
bool pathfinder::build_costmap()
{
	INFO_TARGET << "Pathfinder build costmap: (" << m_limits.X.min << ","
												<< m_limits.Z.min << ") ("
												<< m_limits.X.max << ","
												<< m_limits.Z.max << ")"
												<< std::endl;
	m_data.resize(m_max_index_x);
	for (int x = 0; x < m_max_index_x; x++) {
		m_data[x].resize(m_max_index_z);
		for (int z = 0; z < m_max_index_z; z++) {
			m_data[x][z].resize(m_max_index_y);

			int surfaces = 0;
			for (int y = 0; y < m_max_index_y; y++) {
				v3s16 ipos(x,y,z);

				v3s16 realpos = getRealPos(ipos);

				MapNode current = m_env->getMap().getNodeNoEx(realpos);
				MapNode below   = m_env->getMap().getNodeNoEx(realpos + v3s16(0,-1,0));


				if ((current.param0 == CONTENT_IGNORE) ||
						(below.param0 == CONTENT_IGNORE)) {
					DEBUG_OUT("Pathfinder: " << PPOS(realpos) <<
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
						DEBUG_OUT("Pathfinder: " << PPOS(realpos)
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

				surfaces++;

				m_data[x][z][y].valid  = true;
				m_data[x][z][y].pos    = realpos;
				m_data[x][z][y].type   = 'g';
				DEBUG_OUT(x << "," << y << "," << z << ": " << 'a' << std::endl);

				if (m_prefetch) {
				m_data[x][z][y].directions[DIR_XP] =
											calc_cost(realpos,v3s16( 1,0, 0));
				m_data[x][z][y].directions[DIR_XM] =
											calc_cost(realpos,v3s16(-1,0, 0));
				m_data[x][z][y].directions[DIR_ZP] =
											calc_cost(realpos,v3s16( 0,0, 1));
				m_data[x][z][y].directions[DIR_ZM] =
											calc_cost(realpos,v3s16( 0,0,-1));
				}

			}

			if (surfaces >= 1 ) {
				for (int y = 0; y < m_max_index_y; y++) {
					if (m_data[x][z][y].valid) {
						m_data[x][z][y].surfaces = surfaces;
					}
				}
			}
		}
	}
	return true;
}

/******************************************************************************/
path_cost pathfinder::calc_cost(v3s16 pos,v3s16 dir) {
	path_cost retval;

	retval.updated = true;

	v3s16 pos2 = pos + dir;

	//check limits
	if (    (pos2.X < m_limits.X.min) ||
			(pos2.X >= m_limits.X.max) ||
			(pos2.Z < m_limits.Z.min) ||
			(pos2.Z >= m_limits.Z.max)) {
		DEBUG_OUT("Pathfinder: " << PPOS(pos2) <<
				" no cost -> out of limits" << std::endl);
		return retval;
	}

	MapNode node_at_pos2 = m_env->getMap().getNodeNoEx(pos2);

	//did we get information about node?
	if (node_at_pos2.param0 == CONTENT_IGNORE ) {
			VERBOSE_TARGET << "Pathfinder: (1) area at pos: "
					<< PPOS(pos2) << " not loaded";
			return retval;
	}

	if (node_at_pos2.param0 == CONTENT_AIR) {
		MapNode node_below_pos2 =
							m_env->getMap().getNodeNoEx(pos2 + v3s16(0,-1,0));

		//did we get information about node?
		if (node_below_pos2.param0 == CONTENT_IGNORE ) {
				VERBOSE_TARGET << "Pathfinder: (2) area at pos: "
					<< PPOS((pos2 + v3s16(0,-1,0))) << " not loaded";
				return retval;
		}

		if (node_below_pos2.param0 != CONTENT_AIR) {
			retval.valid = true;
			retval.value = 1;
			retval.direction = 0;
			DEBUG_OUT("Pathfinder: "<< PPOS(pos)
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
					DEBUG_OUT("Pathfinder cost below height found" << std::endl);
				}
				else {
					INFO_TARGET << "Pathfinder:"
							" distance to surface below to big: "
							<< (testpos.Y - pos2.Y) << " max: " << m_maxdrop
							<< std::endl;
				}
			}
			else {
				DEBUG_OUT("Pathfinder: no surface below found" << std::endl);
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
				DEBUG_OUT("Pathfinder cost above found" << std::endl);
			}
			else {
				DEBUG_OUT("Pathfinder: distance to surface above to big: "
						<< (testpos.Y - pos2.Y) << " max: " << m_maxjump
						<< std::endl);
			}
		}
		else {
			DEBUG_OUT("Pathfinder: no surface above found" << std::endl);
		}
	}
	return retval;
}

/******************************************************************************/
v3s16 pathfinder::getIndexPos(v3s16 pos) {

	v3s16 retval = pos;
	retval.X -= m_limits.X.min;
	retval.Y -= m_limits.Y.min;
	retval.Z -= m_limits.Z.min;

	return retval;
}

/******************************************************************************/
path_gridnode& pathfinder::getIndexElement(v3s16 ipos) {
	return m_data[ipos.X][ipos.Z][ipos.Y];
}

/******************************************************************************/
bool pathfinder::valid_index(v3s16 index) {
	if (	(index.X < m_max_index_x) &&
			(index.Y < m_max_index_y) &&
			(index.Z < m_max_index_z) &&
			(index.X >= 0) &&
			(index.Y >= 0) &&
			(index.Z >= 0))
		return true;

	return false;
}

/******************************************************************************/
v3s16 pathfinder::invert(v3s16 pos) {
	v3s16 retval = pos;

	retval.X *=-1;
	retval.Y *=-1;
	retval.Z *=-1;

	return retval;
}

/******************************************************************************/
bool pathfinder::update_all_costs(	v3s16 ipos,
									v3s16 srcdir,
									int current_cost,
									int level) {

	path_gridnode& g_pos = getIndexElement(ipos);
	g_pos.totalcost = current_cost;
	g_pos.sourcedir = srcdir;

	level ++;

	//check if target has been found
	if (g_pos.target) {
		m_min_target_distance = current_cost;
		DEBUG_OUT(LVL " Pathfinder: target found!" << std::endl);
		return true;
	}

	bool retval = false;

	std::vector<v3s16> directions;

	directions.push_back(v3s16( 1,0, 0));
	directions.push_back(v3s16(-1,0, 0));
	directions.push_back(v3s16( 0,0, 1));
	directions.push_back(v3s16( 0,0,-1));

	for (unsigned int i=0; i < directions.size(); i++) {
		if (directions[i] != srcdir) {
			path_cost cost = g_pos.get_cost(directions[i]);

			if (cost.valid) {
				directions[i].Y = cost.direction;

				v3s16 ipos2 = ipos + directions[i];

				if (!valid_index(ipos2)) {
					DEBUG_OUT(LVL " Pathfinder: " << PPOS(ipos2) <<
							" out of range (" << m_limits.X.max << "," <<
							m_limits.Y.max << "," << m_limits.Z.max
							<<")" << std::endl);
					continue;
				}

				path_gridnode& g_pos2 = getIndexElement(ipos2);

				if (!g_pos2.valid) {
					VERBOSE_TARGET << LVL "Pathfinder: no data for new position: "
												<< PPOS(ipos2) << std::endl;
					continue;
				}

				assert(cost.value > 0);

				int new_cost = current_cost + cost.value;

				// check if there already is a smaller path
				if ((m_min_target_distance > 0) &&
						(m_min_target_distance < new_cost)) {
					return false;
				}

				if ((g_pos2.totalcost < 0) ||
						(g_pos2.totalcost > new_cost)) {
					DEBUG_OUT(LVL "Pathfinder: updating path at: "<<
							PPOS(ipos2) << " from: " << g_pos2.totalcost << " to "<<
							new_cost << std::endl);
					if (update_all_costs(ipos2,invert(directions[i]),
											new_cost,level)) {
						retval = true;
						}
					}
				else {
					DEBUG_OUT(LVL "Pathfinder:"
							" already found shorter path to: "
							<< PPOS(ipos2) << std::endl);
				}
			}
			else {
				DEBUG_OUT(LVL "Pathfinder:"
						" not moving to invalid direction: "
						<< PPOS(directions[i]) << std::endl);
			}
		}
	}
	return retval;
}

/******************************************************************************/
int pathfinder::get_manhattandistance(v3s16 pos) {

	int min_x = MYMIN(pos.X,m_destination.X);
	int max_x = MYMAX(pos.X,m_destination.X);
	int min_z = MYMIN(pos.Z,m_destination.Z);
	int max_z = MYMAX(pos.Z,m_destination.Z);

	return (max_x - min_x) + (max_z - min_z);
}

/******************************************************************************/
v3s16 pathfinder::get_dir_heuristic(std::vector<v3s16>& directions,path_gridnode& g_pos) {
	int   minscore = -1;
	v3s16 retdir   = v3s16(0,0,0);
	v3s16 srcpos = g_pos.pos;
	DEBUG_OUT("Pathfinder: remaining dirs at beginning:"
				<< directions.size() << std::endl);

	for (std::vector<v3s16>::iterator iter = directions.begin();
			iter != directions.end();
			iter ++) {

		v3s16 pos1 =  v3s16(srcpos.X + iter->X,0,srcpos.Z+iter->Z);

		int cur_manhattan = get_manhattandistance(pos1);
		path_cost cost    = g_pos.get_cost(*iter);

		if (!cost.updated) {
			cost = calc_cost(g_pos.pos,*iter);
			g_pos.set_cost(*iter,cost);
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
					iter ++) {
			if(*iter == retdir) {
				DEBUG_OUT("Pathfinder: removing return direction" << std::endl);
				directions.erase(iter);
				break;
			}
		}
	}
	else {
		DEBUG_OUT("Pathfinder: didn't find any valid direction clearing"
					<< std::endl);
		directions.clear();
	}
	DEBUG_OUT("Pathfinder: remaining dirs at end:" << directions.size()
				<< std::endl);
	return retdir;
}

/******************************************************************************/
bool pathfinder::update_cost_heuristic(	v3s16 ipos,
									v3s16 srcdir,
									int current_cost,
									int level) {

	path_gridnode& g_pos = getIndexElement(ipos);
	g_pos.totalcost = current_cost;
	g_pos.sourcedir = srcdir;

	level ++;

	//check if target has been found
	if (g_pos.target) {
		m_min_target_distance = current_cost;
		DEBUG_OUT(LVL " Pathfinder: target found!" << std::endl);
		return true;
	}

	bool retval = false;

	std::vector<v3s16> directions;

	directions.push_back(v3s16( 1,0, 0));
	directions.push_back(v3s16(-1,0, 0));
	directions.push_back(v3s16( 0,0, 1));
	directions.push_back(v3s16( 0,0,-1));

	v3s16 direction = get_dir_heuristic(directions,g_pos);

	while (direction != v3s16(0,0,0) && (!retval)) {

		if (direction != srcdir) {
			path_cost cost = g_pos.get_cost(direction);

			if (cost.valid) {
				direction.Y = cost.direction;

				v3s16 ipos2 = ipos + direction;

				if (!valid_index(ipos2)) {
					DEBUG_OUT(LVL " Pathfinder: " << PPOS(ipos2) <<
							" out of range (" << m_limits.X.max << "," <<
							m_limits.Y.max << "," << m_limits.Z.max
							<<")" << std::endl);
					direction = get_dir_heuristic(directions,g_pos);
					continue;
				}

				path_gridnode& g_pos2 = getIndexElement(ipos2);

				if (!g_pos2.valid) {
					VERBOSE_TARGET << LVL "Pathfinder: no data for new position: "
												<< PPOS(ipos2) << std::endl;
					direction = get_dir_heuristic(directions,g_pos);
					continue;
				}

				assert(cost.value > 0);

				int new_cost = current_cost + cost.value;

				// check if there already is a smaller path
				if ((m_min_target_distance > 0) &&
						(m_min_target_distance < new_cost)) {
					DEBUG_OUT(LVL "Pathfinder:"
							" already longer than best already found path "
							<< PPOS(ipos2) << std::endl);
					return false;
				}

				if ((g_pos2.totalcost < 0) ||
						(g_pos2.totalcost > new_cost)) {
					DEBUG_OUT(LVL "Pathfinder: updating path at: "<<
							PPOS(ipos2) << " from: " << g_pos2.totalcost << " to "<<
							new_cost << " srcdir=" <<
							PPOS(invert(direction))<< std::endl);
					if (update_cost_heuristic(ipos2,invert(direction),
											new_cost,level)) {
						retval = true;
						}
					}
				else {
					DEBUG_OUT(LVL "Pathfinder:"
							" already found shorter path to: "
							<< PPOS(ipos2) << std::endl);
				}
			}
			else {
				DEBUG_OUT(LVL "Pathfinder:"
						" not moving to invalid direction: "
						<< PPOS(direction) << std::endl);
			}
		}
		else {
			DEBUG_OUT(LVL "Pathfinder:"
							" skipping srcdir: "
							<< PPOS(direction) << std::endl);
		}
		direction = get_dir_heuristic(directions,g_pos);
	}
	return retval;
}

/******************************************************************************/
void pathfinder::build_path(std::vector<v3s16>& path,v3s16 pos, int level) {
	level ++;
	if (level > 700) {
		ERROR_TARGET
		<< LVL "Pathfinder: path is too long aborting" << std::endl;
		return;
	}

	path_gridnode& g_pos = getIndexElement(pos);
	if (!g_pos.valid) {
		ERROR_TARGET
		<< LVL "Pathfinder: invalid next pos detected aborting" << std::endl;
		return;
	}

	g_pos.is_element = true;

	//check if source reached
	if (g_pos.source) {
		path.push_back(pos);
		return;
	}

	build_path(path,pos + g_pos.sourcedir,level);
	path.push_back(pos);
}

/******************************************************************************/
v3f pathfinder::tov3f(v3s16 pos) {
	return v3f(BS*pos.X,BS*pos.Y,BS*pos.Z);
}

#ifdef PATHFINDER_DEBUG

/******************************************************************************/
void pathfinder::print_cost() {
	print_cost(DIR_XP);
	print_cost(DIR_XM);
	print_cost(DIR_ZP);
	print_cost(DIR_ZM);
}

/******************************************************************************/
void pathfinder::print_ydir() {
	print_ydir(DIR_XP);
	print_ydir(DIR_XM);
	print_ydir(DIR_ZP);
	print_ydir(DIR_ZM);
}

/******************************************************************************/
void pathfinder::print_cost(path_directions dir) {

	std::cout << "Cost in direction: " << dir_to_name(dir) << std::endl;
	std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
	std::cout << std::setfill(' ');
	for (int y = 0; y < m_max_index_y; y++) {

		std::cout << "Level: " << y << std::endl;

		std::cout << std::setw(4) << " " << "  ";
		for (int x = 0; x < m_max_index_x; x++) {
			std::cout << std::setw(4) << x;
		}
		std::cout << std::endl;

		for (int z = 0; z < m_max_index_z; z++) {
			std::cout << std::setw(4) << z <<": ";
			for (int x = 0; x < m_max_index_x; x++) {
				if (m_data[x][z][y].directions[dir].valid)
					std::cout << std::setw(4)
						<< m_data[x][z][y].directions[dir].value;
				else
					std::cout << std::setw(4) << "-";
				}
			std::cout << std::endl;
		}
		std::cout << std::endl;
	}
}

/******************************************************************************/
void pathfinder::print_ydir(path_directions dir) {

	std::cout << "Height difference in direction: " << dir_to_name(dir) << std::endl;
	std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
	std::cout << std::setfill(' ');
	for (int y = 0; y < m_max_index_y; y++) {

		std::cout << "Level: " << y << std::endl;

		std::cout << std::setw(4) << " " << "  ";
		for (int x = 0; x < m_max_index_x; x++) {
			std::cout << std::setw(4) << x;
		}
		std::cout << std::endl;

		for (int z = 0; z < m_max_index_z; z++) {
			std::cout << std::setw(4) << z <<": ";
			for (int x = 0; x < m_max_index_x; x++) {
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
void pathfinder::print_type() {
	std::cout << "Type of node:" << std::endl;
	std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
	std::cout << std::setfill(' ');
	for (int y = 0; y < m_max_index_y; y++) {

		std::cout << "Level: " << y << std::endl;

		std::cout << std::setw(3) << " " << "  ";
		for (int x = 0; x < m_max_index_x; x++) {
			std::cout << std::setw(3) << x;
		}
		std::cout << std::endl;

		for (int z = 0; z < m_max_index_z; z++) {
			std::cout << std::setw(3) << z <<": ";
			for (int x = 0; x < m_max_index_x; x++) {
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
void pathfinder::print_pathlen() {
	std::cout << "Pathlen:" << std::endl;
		std::cout << std::setfill('-') << std::setw(80) << "-" << std::endl;
		std::cout << std::setfill(' ');
		for (int y = 0; y < m_max_index_y; y++) {

			std::cout << "Level: " << y << std::endl;

			std::cout << std::setw(3) << " " << "  ";
			for (int x = 0; x < m_max_index_x; x++) {
				std::cout << std::setw(3) << x;
			}
			std::cout << std::endl;

			for (int z = 0; z < m_max_index_z; z++) {
				std::cout << std::setw(3) << z <<": ";
				for (int x = 0; x < m_max_index_x; x++) {
					std::cout << std::setw(3) << m_data[x][z][y].totalcost;
				}
				std::cout << std::endl;
			}
			std::cout << std::endl;
		}
		std::cout << std::endl;
}

/******************************************************************************/
std::string pathfinder::dir_to_name(path_directions dir) {
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
void pathfinder::print_path(std::vector<v3s16> path) {

	unsigned int current = 0;
	for (std::vector<v3s16>::iterator i = path.begin();
			i != path.end(); i++) {
		std::cout << std::setw(3) << current << ":" << PPOS((*i)) << std::endl;
		current++;
	}
}

#endif
