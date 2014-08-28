/*
Minetest
Copyright (C) 2010-2013 celeron55, Perttu Ahola <celeron55@gmail.com>,
			  2012-2013 RealBadAngel, Maciej Kasatkin <mk@realbadangel.pl>
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

#ifndef TREEGEN_HEADER
#define TREEGEN_HEADER

#include <matrix4.h>
#include "noise.h"

class ManualMapVoxelManipulator;
class INodeDefManager;
class ServerEnvironment;


namespace treegen {

	enum error {
		SUCCESS,
		UNBALANCED_BRACKETS
	};

	struct TreeDef {
		std::string initial_axiom;
		std::string rules_a;
		std::string rules_b;
		std::string rules_c;
		std::string rules_d;

		MapNode trunknode;
		MapNode leavesnode;
		MapNode leaves2node;

		int leaves2_chance;
		int angle;
		int iterations;
		int iterations_random_level;
		std::string trunk_type;
		bool thin_branches;
		MapNode fruitnode;
		int fruit_chance;
		int seed;
		bool explicit_seed;
	};

	// Add default tree
	void make_tree(ManualMapVoxelManipulator &vmanip, v3s16 p0,
		bool is_apple_tree, INodeDefManager *ndef, int seed);
	// Add jungle tree
	void make_jungletree(VoxelManipulator &vmanip, v3s16 p0,
		INodeDefManager *ndef, int seed);

	// Add L-Systems tree (used by engine)
	treegen::error make_ltree(ManualMapVoxelManipulator &vmanip, v3s16 p0, INodeDefManager *ndef,
		TreeDef tree_definition);
	// Spawn L-systems tree from LUA
	treegen::error spawn_ltree (ServerEnvironment *env, v3s16 p0, INodeDefManager *ndef,
		TreeDef tree_definition);

	// L-System tree gen helper functions
	void tree_node_placement(ManualMapVoxelManipulator &vmanip, v3f p0,
		MapNode node);
	void tree_trunk_placement(ManualMapVoxelManipulator &vmanip, v3f p0,
		TreeDef &tree_definition);
	void tree_leaves_placement(ManualMapVoxelManipulator &vmanip, v3f p0,
		PseudoRandom ps, TreeDef &tree_definition);
	void tree_single_leaves_placement(ManualMapVoxelManipulator &vmanip, v3f p0,
		PseudoRandom ps, TreeDef &tree_definition);
	void tree_fruit_placement(ManualMapVoxelManipulator &vmanip, v3f p0,
		TreeDef &tree_definition);
	irr::core::matrix4 setRotationAxisRadians(irr::core::matrix4 M, double angle, v3f axis);

	v3f transposeMatrix(irr::core::matrix4 M ,v3f v);

}; // namespace treegen
#endif
