/*
Minetest-c55
Copyright (C) 2010-2012 celeron55, Perttu Ahola <celeron55@gmail.com>,
				   2012 RealBadAngel, Maciej Kasatkin <mk@realbadangel.pl>
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

#include "irr_v3d.h"
#include <stack>
#include "util/numeric.h"
#include "util/mathconstants.h"
#include "noise.h"
#include "map.h"
#include "environment.h"
#include "nodedef.h"
#include "treegen.h"

namespace treegen
{

void make_tree(ManualMapVoxelManipulator &vmanip, v3s16 p0,
		bool is_apple_tree, INodeDefManager *ndef)
{
	MapNode treenode(ndef->getId("mapgen_tree"));
	MapNode leavesnode(ndef->getId("mapgen_leaves"));
	MapNode applenode(ndef->getId("mapgen_apple"));

	s16 trunk_h = myrand_range(4, 5);
	v3s16 p1 = p0;
	for(s16 ii=0; ii<trunk_h; ii++)
	{
		if(vmanip.m_area.contains(p1))
			if(ii == 0 || vmanip.getNodeNoExNoEmerge(p1).getContent() == CONTENT_AIR)
				vmanip.m_data[vmanip.m_area.index(p1)] = treenode;
		p1.Y++;
	}

	// p1 is now the last piece of the trunk
	p1.Y -= 1;

	VoxelArea leaves_a(v3s16(-2,-1,-2), v3s16(2,2,2));
	//SharedPtr<u8> leaves_d(new u8[leaves_a.getVolume()]);
	Buffer<u8> leaves_d(leaves_a.getVolume());
	for(s32 i=0; i<leaves_a.getVolume(); i++)
		leaves_d[i] = 0;

	// Force leaves at near the end of the trunk
	{
		s16 d = 1;
		for(s16 z=-d; z<=d; z++)
		for(s16 y=-d; y<=d; y++)
		for(s16 x=-d; x<=d; x++)
		{
			leaves_d[leaves_a.index(v3s16(x,y,z))] = 1;
		}
	}

	// Add leaves randomly
	for(u32 iii=0; iii<7; iii++)
	{
		s16 d = 1;

		v3s16 p(
			myrand_range(leaves_a.MinEdge.X, leaves_a.MaxEdge.X-d),
			myrand_range(leaves_a.MinEdge.Y, leaves_a.MaxEdge.Y-d),
			myrand_range(leaves_a.MinEdge.Z, leaves_a.MaxEdge.Z-d)
		);

		for(s16 z=0; z<=d; z++)
		for(s16 y=0; y<=d; y++)
		for(s16 x=0; x<=d; x++)
		{
			leaves_d[leaves_a.index(p+v3s16(x,y,z))] = 1;
		}
	}

	// Blit leaves to vmanip
	for(s16 z=leaves_a.MinEdge.Z; z<=leaves_a.MaxEdge.Z; z++)
	for(s16 y=leaves_a.MinEdge.Y; y<=leaves_a.MaxEdge.Y; y++)
	for(s16 x=leaves_a.MinEdge.X; x<=leaves_a.MaxEdge.X; x++)
	{
		v3s16 p(x,y,z);
		p += p1;
		if(vmanip.m_area.contains(p) == false)
			continue;
		u32 vi = vmanip.m_area.index(p);
		if(vmanip.m_data[vi].getContent() != CONTENT_AIR
				&& vmanip.m_data[vi].getContent() != CONTENT_IGNORE)
			continue;
		u32 i = leaves_a.index(x,y,z);
		if(leaves_d[i] == 1) {
			bool is_apple = myrand_range(0,99) < 10;
			if(is_apple_tree && is_apple) {
				vmanip.m_data[vi] = applenode;
			} else {
				vmanip.m_data[vi] = leavesnode;
			}
		}
	}
}

// L-System tree LUA spawner
void spawn_ltree (ServerEnvironment *env, v3s16 p0, INodeDefManager *ndef, TreeDef tree_definition)
{
	ServerMap *map = &env->getServerMap();
	core::map<v3s16, MapBlock*> modified_blocks;
	ManualMapVoxelManipulator vmanip(map);
	v3s16 tree_blockp = getNodeBlockPos(p0);
	vmanip.initialEmerge(tree_blockp - v3s16(1,1,1), tree_blockp + v3s16(1,1,1));
	make_ltree (vmanip, p0, ndef, tree_definition);
	vmanip.blitBackAll(&modified_blocks);

	// update lighting
	core::map<v3s16, MapBlock*> lighting_modified_blocks;
	for(core::map<v3s16, MapBlock*>::Iterator
		i = modified_blocks.getIterator();
		i.atEnd() == false; i++)
	{
		lighting_modified_blocks.insert(i.getNode()->getKey(), i.getNode()->getValue());
	}
	map->updateLighting(lighting_modified_blocks, modified_blocks);
	// Send a MEET_OTHER event
	MapEditEvent event;
	event.type = MEET_OTHER;
	for(core::map<v3s16, MapBlock*>::Iterator
		i = modified_blocks.getIterator();
		i.atEnd() == false; i++)
	{
		v3s16 p = i.getNode()->getKey();
		event.modified_blocks.insert(p, true);
	}
	map->dispatchEvent(&event);
}

//L-System tree generator
void make_ltree(ManualMapVoxelManipulator &vmanip, v3s16 p0, INodeDefManager *ndef,
		TreeDef tree_definition)
{
	MapNode dirtnode(ndef->getId("mapgen_dirt"));

	// chance of inserting abcd rules
	double prop_a = 9;
	double prop_b = 8;
	double prop_c = 7;
	double prop_d = 6;

	//randomize tree growth level, minimum=2
	s16 iterations = tree_definition.iterations;
	if (tree_definition.iterations_random_level>0)
		iterations -= myrand_range(0,tree_definition.iterations_random_level);
	if (iterations<2)
		iterations=2;

	s16 MAX_ANGLE_OFFSET = 5;
	double angle_in_radians = (double)tree_definition.angle*M_PI/180;
	double angleOffset_in_radians = (s16)(myrand_range(0,1)%MAX_ANGLE_OFFSET)*M_PI/180;

	//initialize rotation matrix, position and stacks for branches
	core::matrix4 rotation;
	rotation=setRotationAxisRadians(rotation, M_PI/2,v3f(0,0,1));
	v3f position = v3f(0,0,0);
	std::stack <core::matrix4> stack_orientation;
	std::stack <v3f> stack_position;

	//generate axiom
	std::string axiom = tree_definition.initial_axiom;
	for(s16 i=0; i<iterations; i++)
	{
		std::string temp = "";
		for(s16 j=0; j<(s16)axiom.size(); j++)
		{
			char axiom_char = axiom.at(j);
			switch (axiom_char)
			{
			case 'A':
				temp+=tree_definition.rules_a;
				break;
			case 'B':
				temp+=tree_definition.rules_b;
				break;
			case 'C':
				temp+=tree_definition.rules_c;
				break;
			case 'D':
				temp+=tree_definition.rules_d;
				break;
			case 'a':
				if (prop_a >= myrand_range(1,10))
					temp+=tree_definition.rules_a;
				break;
			case 'b':
				if (prop_b >= myrand_range(1,10))
					temp+=tree_definition.rules_b;
				break;
			case 'c':
				if (prop_c >= myrand_range(1,10))
					temp+=tree_definition.rules_c;
				break;
			case 'd':
				if (prop_d >= myrand_range(1,10))
					temp+=tree_definition.rules_d;
				break;
			default:
				temp+=axiom_char;
				break;
			}
		}
		axiom=temp;
	}

	//make sure tree is not floating in the air
	if (tree_definition.thin_trunks == false)
	{
		make_tree_node_placement(vmanip,v3f(p0.X+position.X+1,p0.Y+position.Y-1,p0.Z+position.Z),dirtnode);
		make_tree_node_placement(vmanip,v3f(p0.X+position.X-1,p0.Y+position.Y-1,p0.Z+position.Z),dirtnode);
		make_tree_node_placement(vmanip,v3f(p0.X+position.X,p0.Y+position.Y-1,p0.Z+position.Z+1),dirtnode);
		make_tree_node_placement(vmanip,v3f(p0.X+position.X,p0.Y+position.Y-1,p0.Z+position.Z-1),dirtnode);
	}

	/* build tree out of generated axiom

	Key for Special L-System Symbols used in Axioms

    G  - move forward one unit with the pin down
    F  - move forward one unit with the pin up
    A  - replace with rules set A
    B  - replace with rules set B
    C  - replace with rules set C
    D  - replace with rules set D
    a  - replace with rules set A, chance 90%
    b  - replace with rules set B, chance 80%
    c  - replace with rules set C, chance 70%
    d  - replace with rules set D, chance 60%
    +  - yaw the turtle right by angle degrees
    -  - yaw the turtle left by angle degrees
    &  - pitch the turtle down by angle degrees
    ^  - pitch the turtle up by angle degrees
    /  - roll the turtle to the right by angle degrees
    *  - roll the turtle to the left by angle degrees
    [  - save in stack current state info
    ]  - recover from stack state info

    */

	s16 x,y,z;
	for(s16 i=0; i<(s16)axiom.size(); i++)
	{
		char axiom_char = axiom.at(i);
		core::matrix4 temp_rotation;
		temp_rotation.makeIdentity();
		v3f dir;
		switch (axiom_char)
		{
		case 'G':
			dir = v3f(-1,0,0);
			dir = transposeMatrix(rotation,dir);
			position+=dir;
			break;
		case 'F':
			make_tree_trunk_placement(vmanip,v3f(p0.X+position.X,p0.Y+position.Y,p0.Z+position.Z),tree_definition);
			if (tree_definition.thin_trunks == false)
			{
				make_tree_trunk_placement(vmanip,v3f(p0.X+position.X+1,p0.Y+position.Y,p0.Z+position.Z),tree_definition);
				make_tree_trunk_placement(vmanip,v3f(p0.X+position.X-1,p0.Y+position.Y,p0.Z+position.Z),tree_definition);
				make_tree_trunk_placement(vmanip,v3f(p0.X+position.X,p0.Y+position.Y,p0.Z+position.Z+1),tree_definition);
				make_tree_trunk_placement(vmanip,v3f(p0.X+position.X,p0.Y+position.Y,p0.Z+position.Z-1),tree_definition);
			}
			if (stack_orientation.empty() == false)
			{
				s16 size = 1;
				for(x=-size; x<size+1; x++)
					for(y=-size; y<size+1; y++)
						for(z=-size; z<size+1; z++)
							if (abs(x) == size && abs(y) == size && abs(z) == size)
							{
								make_tree_leaves_placement(vmanip,v3f(p0.X+position.X+x+1,p0.Y+position.Y+y,p0.Z+position.Z+z),tree_definition);
								make_tree_leaves_placement(vmanip,v3f(p0.X+position.X+x-1,p0.Y+position.Y+y,p0.Z+position.Z+z),tree_definition);
								make_tree_leaves_placement(vmanip,v3f(p0.X+position.X+x,p0.Y+position.Y+y,p0.Z+position.Z+z+1),tree_definition);
								make_tree_leaves_placement(vmanip,v3f(p0.X+position.X+x,p0.Y+position.Y+y,p0.Z+position.Z+z-1),tree_definition);
							}
			}
			dir = v3f(1,0,0);
			dir = transposeMatrix(rotation,dir);
			position+=dir;
			break;
		// turtle commands
		case '[':
			stack_orientation.push(rotation);
			stack_position.push(position);
			break;
		case ']':
			rotation=stack_orientation.top();
			stack_orientation.pop();
			position=stack_position.top();
			stack_position.pop();
			break;
		case '+':
			temp_rotation.makeIdentity();
			temp_rotation=setRotationAxisRadians(temp_rotation, angle_in_radians+angleOffset_in_radians,v3f(0,0,1));
			rotation*=temp_rotation;
			break;
		case '-':
			temp_rotation.makeIdentity();
			temp_rotation=setRotationAxisRadians(temp_rotation, angle_in_radians+angleOffset_in_radians,v3f(0,0,-1));
			rotation*=temp_rotation;
			break;
		case '&':
			temp_rotation.makeIdentity();
			temp_rotation=setRotationAxisRadians(temp_rotation, angle_in_radians+angleOffset_in_radians,v3f(0,1,0));
			rotation*=temp_rotation;
			break;
		case '^':
			temp_rotation.makeIdentity();
			temp_rotation=setRotationAxisRadians(temp_rotation, angle_in_radians+angleOffset_in_radians,v3f(0,-1,0));
			rotation*=temp_rotation;
			break;
		case '*':
			temp_rotation.makeIdentity();
			temp_rotation=setRotationAxisRadians(temp_rotation, angle_in_radians,v3f(1,0,0));
			rotation*=temp_rotation;
			break;
		case '/':
			temp_rotation.makeIdentity();
			temp_rotation=setRotationAxisRadians(temp_rotation, angle_in_radians,v3f(-1,0,0));
			rotation*=temp_rotation;
			break;
		default:
			break;
		}
	}
}

void make_tree_node_placement(ManualMapVoxelManipulator &vmanip, v3f p0,
		MapNode node)
{
	v3s16 p1 = v3s16(myround(p0.X),myround(p0.Y),myround(p0.Z));
	if(vmanip.m_area.contains(p1) == false)
		return;
	u32 vi = vmanip.m_area.index(p1);
	if(vmanip.m_data[vi].getContent() != CONTENT_AIR
			&& vmanip.m_data[vi].getContent() != CONTENT_IGNORE)
		return;
	vmanip.m_data[vmanip.m_area.index(p1)] = node;
}

void make_tree_trunk_placement(ManualMapVoxelManipulator &vmanip, v3f p0,
		TreeDef &tree_definition)
{
	v3s16 p1 = v3s16(myround(p0.X),myround(p0.Y),myround(p0.Z));
	if(vmanip.m_area.contains(p1) == false)
		return;
	u32 vi = vmanip.m_area.index(p1);
	if(vmanip.m_data[vi].getContent() != CONTENT_AIR
			&& vmanip.m_data[vi].getContent() != CONTENT_IGNORE)
		return;
	vmanip.m_data[vmanip.m_area.index(p1)] = tree_definition.trunknode;
}

void make_tree_leaves_placement(ManualMapVoxelManipulator &vmanip, v3f p0,
		TreeDef &tree_definition)
{
	v3s16 p1 = v3s16(myround(p0.X),myround(p0.Y),myround(p0.Z));
	if(vmanip.m_area.contains(p1) == false)
		return;
	u32 vi = vmanip.m_area.index(p1);
	if(vmanip.m_data[vi].getContent() != CONTENT_AIR
			&& vmanip.m_data[vi].getContent() != CONTENT_IGNORE)
		return;
	if (tree_definition.fruit_tree)
	{
		if (myrand_range(1,100) > 90+tree_definition.iterations)
			vmanip.m_data[vmanip.m_area.index(p1)] = tree_definition.fruitnode;
		else
			vmanip.m_data[vmanip.m_area.index(p1)] = tree_definition.leavesnode;
	}
	else if (myrand_range(1,100) > 20)
		vmanip.m_data[vmanip.m_area.index(p1)] = tree_definition.leavesnode;
}

irr::core::matrix4 setRotationAxisRadians(irr::core::matrix4 M, double angle, v3f axis)
{
	double c = cos(angle);
	double s = sin(angle);
	double t = 1.0 - c;

	double tx  = t * axis.X;
	double ty  = t * axis.Y;
	double tz  = t * axis.Z;
	double sx  = s * axis.X;
	double sy  = s * axis.Y;
	double sz  = s * axis.Z;

	M[0] = tx * axis.X + c;
	M[1] = tx * axis.Y + sz;
	M[2] = tx * axis.Z - sy;

	M[4] = ty * axis.X - sz;
	M[5] = ty * axis.Y + c;
	M[6] = ty * axis.Z + sx;

	M[8]  = tz * axis.X + sy;
	M[9]  = tz * axis.Y - sx;
	M[10] = tz * axis.Z + c;
	return M;
}

v3f transposeMatrix(irr::core::matrix4 M, v3f v)
{
	v3f translated;
	double x = M[0] * v.X + M[4] * v.Y + M[8]  * v.Z +M[12];
	double y = M[1] * v.X + M[5] * v.Y + M[9]  * v.Z +M[13];
	double z = M[2] * v.X + M[6] * v.Y + M[10] * v.Z +M[14];
	translated.X=x;
	translated.Y=y;
	translated.Z=z;
	return translated;
}

#if 0
static void make_jungletree(VoxelManipulator &vmanip, v3s16 p0,
		INodeDefManager *ndef)
{
	MapNode treenode(ndef->getId("mapgen_jungletree"));
	MapNode leavesnode(ndef->getId("mapgen_leaves"));

	for(s16 x=-1; x<=1; x++)
	for(s16 z=-1; z<=1; z++)
	{
		if(myrand_range(0, 2) == 0)
			continue;
		v3s16 p1 = p0 + v3s16(x,0,z);
		v3s16 p2 = p0 + v3s16(x,-1,z);
		if(vmanip.m_area.contains(p2)
				&& vmanip.m_data[vmanip.m_area.index(p2)] == CONTENT_AIR)
			vmanip.m_data[vmanip.m_area.index(p2)] = treenode;
		else if(vmanip.m_area.contains(p1))
			vmanip.m_data[vmanip.m_area.index(p1)] = treenode;
	}

	s16 trunk_h = myrand_range(8, 12);
	v3s16 p1 = p0;
	for(s16 ii=0; ii<trunk_h; ii++)
	{
		if(vmanip.m_area.contains(p1))
			vmanip.m_data[vmanip.m_area.index(p1)] = treenode;
		p1.Y++;
	}

	// p1 is now the last piece of the trunk
	p1.Y -= 1;

	VoxelArea leaves_a(v3s16(-3,-2,-3), v3s16(3,2,3));
	//SharedPtr<u8> leaves_d(new u8[leaves_a.getVolume()]);
	Buffer<u8> leaves_d(leaves_a.getVolume());
	for(s32 i=0; i<leaves_a.getVolume(); i++)
		leaves_d[i] = 0;

	// Force leaves at near the end of the trunk
	{
		s16 d = 1;
		for(s16 z=-d; z<=d; z++)
		for(s16 y=-d; y<=d; y++)
		for(s16 x=-d; x<=d; x++)
		{
			leaves_d[leaves_a.index(v3s16(x,y,z))] = 1;
		}
	}

	// Add leaves randomly
	for(u32 iii=0; iii<30; iii++)
	{
		s16 d = 1;

		v3s16 p(
			myrand_range(leaves_a.MinEdge.X, leaves_a.MaxEdge.X-d),
			myrand_range(leaves_a.MinEdge.Y, leaves_a.MaxEdge.Y-d),
			myrand_range(leaves_a.MinEdge.Z, leaves_a.MaxEdge.Z-d)
		);

		for(s16 z=0; z<=d; z++)
		for(s16 y=0; y<=d; y++)
		for(s16 x=0; x<=d; x++)
		{
			leaves_d[leaves_a.index(p+v3s16(x,y,z))] = 1;
		}
	}

	// Blit leaves to vmanip
	for(s16 z=leaves_a.MinEdge.Z; z<=leaves_a.MaxEdge.Z; z++)
	for(s16 y=leaves_a.MinEdge.Y; y<=leaves_a.MaxEdge.Y; y++)
	for(s16 x=leaves_a.MinEdge.X; x<=leaves_a.MaxEdge.X; x++)
	{
		v3s16 p(x,y,z);
		p += p1;
		if(vmanip.m_area.contains(p) == false)
			continue;
		u32 vi = vmanip.m_area.index(p);
		if(vmanip.m_data[vi].getContent() != CONTENT_AIR
				&& vmanip.m_data[vi].getContent() != CONTENT_IGNORE)
			continue;
		u32 i = leaves_a.index(x,y,z);
		if(leaves_d[i] == 1)
			vmanip.m_data[vi] = leavesnode;
	}
}
#endif

}; // namespace treegen