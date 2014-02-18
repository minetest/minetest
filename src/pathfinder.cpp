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
#include <climits>

#define PPOS(pos) "(" << pos.X << "," << pos.Y << "," << pos.Z << ")"

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

OpenElement::OpenElement() :
	f_value(0),
	start_cost(0),
	pos(v3s16(0, 0, 0)),
	prev_pos(v3s16(0, 0, 0))
{
}

OpenElement::OpenElement(unsigned int _f_value, unsigned int _start_cost, v3s16 _pos, v3s16 _prev_pos) :
	f_value(_f_value),
	start_cost(_start_cost),
	pos(_pos),
	prev_pos(_prev_pos)
{
}

OpenElement& OpenElement::operator=(const OpenElement& e)
{
	f_value = e.f_value;
	start_cost = e.start_cost;
	pos = e.pos;
	prev_pos = e.prev_pos;
	return *this;
}

bool OpenElement::operator<(const OpenElement& e) const
{
	return (f_value < e.f_value) || ((f_value == e.f_value) && (start_cost > e.start_cost));
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
		errorstream << "missing environment pointer" << std::endl;
		return retval;
	}

	m_searchdistance = searchdistance;
	m_env = env;
	m_maxjump = max_jump;
	m_maxdrop = max_drop;
	m_start       = source;
	m_destination = destination;
	m_adjacency = adjacency;

	int min_x = std::min(source.X, destination.X);
	int max_x = std::max(source.X, destination.X);

	int min_y = std::min(source.Y, destination.Y);
	int max_y = std::max(source.Y, destination.Y);

	int min_z = std::min(source.Z, destination.Z);
	int max_z = std::max(source.Z, destination.Z);

	m_limits.X.min = min_x - searchdistance;
	m_limits.X.max = max_x + searchdistance;
	m_limits.Y.min = min_y - searchdistance;
	m_limits.Y.max = max_y + searchdistance;
	m_limits.Z.min = min_z - searchdistance;
	m_limits.Z.max = max_z + searchdistance;

	bool update_cost_retval = false;

	switch(algo) {
	case A_STAR:
		update_cost_retval = findPathHeuristic(source, m_adjacency_4, getManhattanDistance);
		break;
	default:
		errorstream << "missing algorithm"<< std::endl;
		break;
	}

	if(update_cost_retval) {
		std::vector<v3s16> path;
		buildPath(path, source, destination);

#ifdef PATHFINDER_CALC_TIME
		timespec ts2;
		clock_gettime(CLOCK_REALTIME, &ts2);

		int ms = (ts2.tv_nsec - ts.tv_nsec)/(1000*1000);
		int us = ((ts2.tv_nsec - ts.tv_nsec) - (ms*1000*1000))/1000;
		int ns = ((ts2.tv_nsec - ts.tv_nsec) - ( (ms*1000*1000) + (us*1000)));


		std::cout << "Calculating path took: " << (ts2.tv_sec - ts.tv_sec) <<
			"s " << ms << "ms " << us << "us " << ns << "ns " << std::endl;
#endif
		return path;
	}


	return retval;
}

/******************************************************************************/
PathFinder::PathFinder() :
	m_searchdistance(0),
	m_maxdrop(0),
	m_maxjump(0),
	m_start(0,0,0),
	m_destination(0,0,0),
	m_limits(),
	m_env(0)
{
	m_adjacency_4.push_back(v3s16(-1, 0, 0));
	m_adjacency_4.push_back(v3s16(1, 0, 0));
	m_adjacency_4.push_back(v3s16(0, 0, 1));
	m_adjacency_4.push_back(v3s16(0, 0, -1));

	m_adjacency_4_cost.push_back(1);
	m_adjacency_4_cost.push_back(1);
	m_adjacency_4_cost.push_back(1);
	m_adjacency_4_cost.push_back(1);

	m_adjacency_8.push_back(v3s16(-1, 0, 0));
	m_adjacency_8.push_back(v3s16(1, 0, 0));
	m_adjacency_8.push_back(v3s16(0, 0, 1));
	m_adjacency_8.push_back(v3s16(0, 0, -1));
	m_adjacency_8.push_back(v3s16(-1, 0, -1));
	m_adjacency_8.push_back(v3s16(1, 0, -1));
	m_adjacency_8.push_back(v3s16(-1, 0, 1));
	m_adjacency_8.push_back(v3s16(1, 0, 1));

	m_adjacency_8_cost.push_back(1);
	m_adjacency_8_cost.push_back(1);
	m_adjacency_8_cost.push_back(1);
	m_adjacency_8_cost.push_back(1);
	m_adjacency_8_cost.push_back(1);
	m_adjacency_8_cost.push_back(1);
	m_adjacency_8_cost.push_back(1);
	m_adjacency_8_cost.push_back(1);
}

unsigned int PathFinder::getDirectionCost(unsigned int id)
{
	switch(m_adjacency) {
	case ADJACENCY_4:
		return m_adjacency_4_cost[id];
	case ADJACENCY_8:
		return m_adjacency_8_cost[id];
	}
	return UINT_MAX;
}

bool PathFinder::findPathHeuristic(v3s16 pos, std::vector <v3s16>& directions,
                                   unsigned int (*heuristicFunction)(v3s16, v3s16))
{
	std::multiset <OpenElement> q;

	used.clear();
	q.insert(OpenElement(heuristicFunction(pos, m_destination), 0, pos, v3s16(0, 0, 0)));
	while(!q.empty()) {
		v3s16 current_pos = q.begin()->pos;
		v3s16 prev_pos = q.begin()->prev_pos;
		unsigned int current_cost = q.begin()->start_cost;
		q.erase(q.begin());
		for(unsigned int i = 0; i < directions.size(); ++i) {
			v3s16 next_pos = current_pos + directions[i];
			unsigned int next_cost = current_cost + getDirectionCost(i);
			// Check limits or already processed
			if((next_pos.X <  m_limits.X.min) ||
			   (next_pos.X >= m_limits.X.max) ||
			   (next_pos.Z <  m_limits.Z.min) ||
			   (next_pos.Z >= m_limits.Z.max)) {
				continue;
			}


			MapNode node_at_next_pos = m_env->getMap().getNodeNoEx(next_pos);

			if(node_at_next_pos.param0 == CONTENT_IGNORE) {
				continue;
			}

			if(node_at_next_pos.param0 == CONTENT_AIR) {
				MapNode node_below_next_pos =
					m_env->getMap().getNodeNoEx(next_pos + v3s16(0, -1, 0));


				if(node_below_next_pos.param0 == CONTENT_IGNORE) {
					continue;
				}

				if(node_below_next_pos.param0 == CONTENT_AIR) {
					// Try jump down
					v3s16 test_pos = next_pos - v3s16(0, -1, 0);
					MapNode node_at_test_pos = m_env->getMap().getNodeNoEx(test_pos);

					while((node_at_test_pos.param0 == CONTENT_AIR) &&
					      (test_pos.Y > m_limits.Y.min)) {
						--test_pos.Y;
						node_at_test_pos = m_env->getMap().getNodeNoEx(test_pos);
					}
					++test_pos.Y;

					if((test_pos.Y >= m_limits.Y.min) &&
					   (node_at_test_pos.param0 != CONTENT_IGNORE) &&
					   (node_at_test_pos.param0 != CONTENT_AIR) &&
					   ((next_pos.Y - test_pos.Y) <= m_maxdrop)) {
						next_pos.Y = test_pos.Y;
						next_cost = current_cost + getDirectionCost(i) * 2;
					} else {
						continue;
					}
				}
			} else {
				// Try jump up
				v3s16 test_pos = next_pos;
				MapNode node_at_test_pos = m_env->getMap().getNodeNoEx(test_pos);

				while((node_at_test_pos.param0 != CONTENT_IGNORE) &&
				      (node_at_test_pos.param0 != CONTENT_AIR) &&
				      (test_pos.Y < m_limits.Y.max)) {
					++test_pos.Y;
					node_at_test_pos = m_env->getMap().getNodeNoEx(test_pos);
				}
 
				// Did we find surface?
				if((test_pos.Y <= m_limits.Y.max) &&
				   (node_at_test_pos.param0 == CONTENT_AIR) &&
				   (test_pos.Y - next_pos.Y <= m_maxjump)) {
					next_pos.Y = test_pos.Y;
					next_cost = current_cost + getDirectionCost(i) * 2;
				} else {
					continue;
				}
			}

			if((used.find(next_pos) == used.end()) || (used[next_pos].second > next_cost)) {
				used[next_pos].first = current_pos;
				used[next_pos].second = next_cost;
				q.insert(OpenElement(next_cost + heuristicFunction(next_pos, m_destination),
				                     next_cost, next_pos, current_pos));
			}
		}
		if(current_pos == m_destination) {
			return true;
		}
	}
	return (used.find(m_destination) != used.end());
}

/******************************************************************************/
void PathFinder::buildPath(std::vector<v3s16>& path, v3s16 start_pos, v3s16 end_pos)
{
	v3s16 current_pos = end_pos;
	v3s16 next_pos;
	while(current_pos != start_pos) {
		path.push_back(current_pos);
		current_pos = used[current_pos].first;
	}
	path.push_back(start_pos); 
}

