/*
Minetest-c55
Copyright (C) 2010 celeron55, Perttu Ahola <celeron55@gmail.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License along
with this program; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef HEIGHTMAP_HEADER
#define HEIGHTMAP_HEADER

#include <iostream>
#include <time.h>
#include <sstream>

#include "debug.h"
#include "common_irrlicht.h"
#include "exceptions.h"
#include "utility.h"
#include "serialization.h"

#define GROUNDHEIGHT_NOTFOUND_SETVALUE (-10e6)
#define GROUNDHEIGHT_VALID_MINVALUE    ( -9e6)

class Heightmappish
{
public:
	virtual f32 getGroundHeight(v2s16 p, bool generate=true) = 0;
	virtual void setGroundHeight(v2s16 p, f32 y, bool generate=true) = 0;
	
	v2f32 getSlope(v2s16 p)
	{
		f32 y0 = getGroundHeight(p, false);

		v2s16 dirs[] = {
			v2s16(1,0),
			v2s16(0,1),
		};

		v2f32 fdirs[] = {
			v2f32(1,0),
			v2f32(0,1),
		};

		v2f32 slopevector(0.0, 0.0);
		
		for(u16 i=0; i<2; i++){
			f32 y1 = 0.0;
			f32 y2 = 0.0;
			f32 count = 0.0;

			v2s16 p1 = p - dirs[i];
			y1 = getGroundHeight(p1, false);
			if(y1 > GROUNDHEIGHT_VALID_MINVALUE){
				y1 -= y0;
				count += 1.0;
			}
			else
				y1 = 0;

			v2s16 p2 = p + dirs[i];
			y2 = getGroundHeight(p2, false);
			if(y2 > GROUNDHEIGHT_VALID_MINVALUE){
				y2 -= y0;
				count += 1.0;
			}
			else
				y2 = 0;

			if(count < 0.001)
				return v2f32(0.0, 0.0);
			
			/*
				If y2 is higher than y1, slope is positive
			*/
			f32 slope = (y2 - y1)/count;

			slopevector += fdirs[i] * slope;
		}

		return slopevector;
	}

};

// TODO: Get rid of this dummy wrapper
class Heightmap : public Heightmappish /*, public ReferenceCounted*/
{
};

class WrapperHeightmap : public Heightmap
{
	Heightmappish *m_target;
public:

	WrapperHeightmap(Heightmappish *target):
		m_target(target)
	{
		if(target == NULL)
			throw NullPointerException();
	}

	f32 getGroundHeight(v2s16 p, bool generate=true)
	{
		return m_target->getGroundHeight(p, generate);
	}
	void setGroundHeight(v2s16 p, f32 y, bool generate=true)
	{
		m_target->setGroundHeight(p, y, generate);
	}
};

/*
	Base class that defines a generator that gives out values at
	positions in 2-dimensional space.
	Can be given to UnlimitedHeightmap to feed stuff.

	These are always serialized as readable text ending in "\n"
*/
class ValueGenerator
{
public:
	ValueGenerator(){}
	virtual ~ValueGenerator(){}
	
	static ValueGenerator* deSerialize(std::string line);
	
	static ValueGenerator* deSerialize(std::istream &is)
	{
		std::string line;
		std::getline(is, line, '\n');
		return deSerialize(line);
	}
	
	void serializeBase(std::ostream &os)
	{
		os<<getName()<<" ";
	}

	// Virtual methods
	virtual const char * getName() const = 0;
	virtual f32 getValue(v2s16 p) = 0;
	virtual void serialize(std::ostream &os) = 0;
};

class ConstantGenerator : public ValueGenerator
{
public:
	f32 m_value;

	ConstantGenerator(f32 value)
	{
		m_value = value;
	}

	const char * getName() const
	{
		return "constant";
	}
	
	f32 getValue(v2s16 p)
	{
		return m_value;
	}

	void serialize(std::ostream &os)
	{
		serializeBase(os);

		std::ostringstream ss;
		//ss.imbue(std::locale("C"));

		ss<<m_value<<"\n";

		os<<ss.str();
	}
};

class LinearGenerator : public ValueGenerator
{
public:
	f32 m_height;
	v2f m_slope;

	LinearGenerator(f32 height, v2f slope)
	{
		m_height = height;
		m_slope = slope;
	}

	const char * getName() const
	{
		return "linear";
	}
	
	f32 getValue(v2s16 p)
	{
		return m_height + m_slope.X * p.X + m_slope.Y * p.Y;
	}

	void serialize(std::ostream &os)
	{
		serializeBase(os);

		std::ostringstream ss;
		//ss.imbue(std::locale("C"));

		ss<<m_height<<" "<<m_slope.X<<" "<<m_slope.Y<<"\n";

		os<<ss.str();
	}
};

class PowerGenerator : public ValueGenerator
{
public:
	f32 m_height;
	v2f m_slope;
	f32 m_power;

	PowerGenerator(f32 height, v2f slope, f32 power)
	{
		m_height = height;
		m_slope = slope;
		m_power = power;
	}

	const char * getName() const
	{
		return "power";
	}
	
	f32 getValue(v2s16 p)
	{
		return m_height
				+ m_slope.X * pow((f32)p.X, m_power)
				+ m_slope.Y * pow((f32)p.Y, m_power);
	}

	void serialize(std::ostream &os)
	{
		serializeBase(os);

		std::ostringstream ss;
		//ss.imbue(std::locale("C"));

		ss<<m_height<<" "
			<<m_slope.X<<" "
			<<m_slope.Y<<" "
			<<m_power<<"\n";

		os<<ss.str();
	}
};

class FixedHeightmap : public Heightmap
{
	// A meta-heightmap on which this heightmap is located
	// (at m_pos_on_master * m_blocksize)
	Heightmap * m_master;
	// Position on master heightmap (in blocks)
	v2s16 m_pos_on_master;
	s32 m_blocksize; // This is (W-1) = (H-1)
	// These are the actual size of the data
	s32 W;
	s32 H;
	f32 *m_data;

public:

	FixedHeightmap(Heightmap * master,
			v2s16 pos_on_master, s32 blocksize):
		m_master(master),
		m_pos_on_master(pos_on_master),
		m_blocksize(blocksize)
	{
		W = m_blocksize+1;
		H = m_blocksize+1;
		m_data = NULL;
		m_data = new f32[(blocksize+1)*(blocksize+1)];

		for(s32 i=0; i<(blocksize+1)*(blocksize+1); i++){
			m_data[i] = GROUNDHEIGHT_NOTFOUND_SETVALUE;
		}
	}

	~FixedHeightmap()
	{
		if(m_data)
			delete[] m_data;
	}

	v2s16 getPosOnMaster()
	{
		return m_pos_on_master;
	}

	/*
		TODO: BorderWrapper class or something to allow defining
		borders that wrap to an another heightmap. The algorithm
		should be allowed to edit stuff over the border and on
		the border in that case, too.
		This will allow non-square heightmaps, too. (probably)
	*/

	void print()
	{
		printf("FixedHeightmap::print(): size is %ix%i\n", W, H);
		for(s32 y=0; y<H; y++){
			for(s32 x=0; x<W; x++){
				/*if(getSeeded(v2s16(x,y)))
					printf("S");*/
				f32 n = getGroundHeight(v2s16(x,y));
				if(n < GROUNDHEIGHT_VALID_MINVALUE)
					printf("  -   ");
				else
					printf("% -5.1f ", getGroundHeight(v2s16(x,y)));
			}
			printf("\n");
		}
	}
	
	bool overborder(v2s16 p)
	{
		return (p.X < 0 || p.X >= W || p.Y < 0 || p.Y >= H);
	}

	bool atborder(v2s16 p)
	{
		if(overborder(p))
			return false;
		return (p.X == 0 || p.X == W-1 || p.Y == 0 || p.Y == H-1);
	}

	void setGroundHeight(v2s16 p, f32 y, bool generate=false)
	{
		/*dstream<<"FixedHeightmap::setGroundHeight(("
				<<p.X<<","<<p.Y
				<<"), "<<y<<")"<<std::endl;*/
		if(overborder(p))
			throw InvalidPositionException();
		m_data[p.Y*W + p.X] = y;
	}
	
	// Returns true on success, false on railure.
	bool setGroundHeightParent(v2s16 p, f32 y, bool generate=false)
	{
		/*// Position on master
		v2s16 blockpos_nodes = m_pos_on_master * m_blocksize;
		v2s16 nodepos_master = blockpos_nodes + p;
		dstream<<"FixedHeightmap::setGroundHeightParent(("
				<<p.X<<","<<p.Y
				<<"), "<<y<<"): nodepos_master=("
				<<nodepos_master.X<<","
				<<nodepos_master.Y<<")"<<std::endl;
		m_master->setGroundHeight(nodepos_master, y, false);*/
		
		// Try to set on master
		bool master_got_it = false;
		if(overborder(p) || atborder(p))
		{
			try{
				// Position on master
				v2s16 blockpos_nodes = m_pos_on_master * m_blocksize;
				v2s16 nodepos_master = blockpos_nodes + p;
				m_master->setGroundHeight(nodepos_master, y, false);

				master_got_it = true;
			}
			catch(InvalidPositionException &e)
			{
			}
		}
		
		if(overborder(p))
			return master_got_it;
		
		setGroundHeight(p, y);

		return true;
	}
	
	f32 getGroundHeight(v2s16 p, bool generate=false)
	{
		if(overborder(p))
			return GROUNDHEIGHT_NOTFOUND_SETVALUE;
		return m_data[p.Y*W + p.X];
	}

	f32 getGroundHeightParent(v2s16 p)
	{
		/*v2s16 blockpos_nodes = m_pos_on_master * m_blocksize;
		return m_master->getGroundHeight(blockpos_nodes + p, false);*/

		if(overborder(p) == false){
			f32 h = getGroundHeight(p);
			if(h > GROUNDHEIGHT_VALID_MINVALUE)
				return h;
		}
		
		// Position on master
		v2s16 blockpos_nodes = m_pos_on_master * m_blocksize;
		f32 h = m_master->getGroundHeight(blockpos_nodes + p, false);
		return h;
	}

	f32 avgNeighbours(v2s16 p, s16 d);

	f32 avgDiagNeighbours(v2s16 p, s16 d);
	
	void makeDiamond(
			v2s16 center,
			s16 a,
			f32 randmax,
			core::map<v2s16, bool> &next_squares);

	void makeSquare(
			v2s16 center,
			s16 a,
			f32 randmax,
			core::map<v2s16, bool> &next_diamonds);
	
	void DiamondSquare(f32 randmax, f32 randfactor);
	
	/*
		corners: [i]=XY: [0]=00, [1]=10, [2]=11, [3]=10
	*/
	void generateContinued(f32 randmax, f32 randfactor, f32 *corners);

	
	static u32 serializedLength(u8 version, u16 blocksize);
	u32 serializedLength(u8 version);
	void serialize(u8 *dest, u8 version);
	void deSerialize(u8 *source, u8 version);
	/*static FixedHeightmap * deSerialize(u8 *source, u32 size,
			u32 &usedsize, Heightmap *master, u8 version);*/
};

class OneChildHeightmap : public Heightmap
{
	s16 m_blocksize;

public:

	FixedHeightmap m_child;

	OneChildHeightmap(s16 blocksize):
		m_blocksize(blocksize),
		m_child(this, v2s16(0,0), blocksize)
	{
	}
	
	f32 getGroundHeight(v2s16 p, bool generate=true)
	{
		if(p.X < 0 || p.X > m_blocksize 
				|| p.Y < 0 || p.Y > m_blocksize)
			return GROUNDHEIGHT_NOTFOUND_SETVALUE;
		return m_child.getGroundHeight(p);
	}
	void setGroundHeight(v2s16 p, f32 y, bool generate=true)
	{
		//dstream<<"OneChildHeightmap::setGroundHeight()"<<std::endl;
		if(p.X < 0 || p.X > m_blocksize 
				|| p.Y < 0 || p.Y > m_blocksize)
			throw InvalidPositionException();
		m_child.setGroundHeight(p, y);
	}
};


/*
	This is a dynamic container of an arbitrary number of heightmaps
	at arbitrary positions.
	
	It is able to redirect queries to the corresponding heightmaps and
	it generates new heightmaps on-the-fly according to the relevant
	parameters.
	
	It doesn't have a master heightmap because it is meant to be used
	as such itself.

	Child heightmaps are spaced at m_blocksize distances, and are of
	size (m_blocksize+1)*(m_blocksize+1)

	This is used as the master heightmap of a Map object.
*/
class UnlimitedHeightmap: public Heightmap
{
private:

	core::map<v2s16, FixedHeightmap*> m_heightmaps;
	s16 m_blocksize;

	ValueGenerator *m_randmax_generator;
	ValueGenerator *m_randfactor_generator;
	ValueGenerator *m_base_generator;

public:

	UnlimitedHeightmap(
			s16 blocksize,
			ValueGenerator *randmax_generator,
			ValueGenerator *randfactor_generator,
			ValueGenerator *base_generator
			):
		m_blocksize(blocksize),
		m_randmax_generator(randmax_generator),
		m_randfactor_generator(randfactor_generator),
		m_base_generator(base_generator)
	{
		assert(m_randmax_generator != NULL);
		assert(m_randfactor_generator != NULL);
		assert(m_base_generator != NULL);
	}

	~UnlimitedHeightmap()
	{
		core::map<v2s16, FixedHeightmap*>::Iterator i;
		i = m_heightmaps.getIterator();
		for(; i.atEnd() == false; i++)
		{
			delete i.getNode()->getValue();
		}

		delete m_randmax_generator;
		delete m_randfactor_generator;
		delete m_base_generator;
	}

	/*void setParams(f32 randmax, f32 randfactor)
	{
		m_randmax = randmax;
		m_randfactor = randfactor;
	}*/
	
	void print();

	v2s16 getNodeHeightmapPos(v2s16 p)
	{
		return v2s16(
				(p.X>=0 ? p.X : p.X-m_blocksize+1) / m_blocksize,
				(p.Y>=0 ? p.Y : p.Y-m_blocksize+1) / m_blocksize);
	}

	// Can throw an InvalidPositionException
	FixedHeightmap * getHeightmap(v2s16 p, bool generate=true);
	
	f32 getGroundHeight(v2s16 p, bool generate=true);
	void setGroundHeight(v2s16 p, f32 y, bool generate=true);
	
	/*static UnlimitedHeightmap * deSerialize(u8 *source, u32 maxsize,
			u32 &usedsize, u8 version);*/
	
	//SharedBuffer<u8> serialize(u8 version);
	void serialize(std::ostream &os, u8 version);
	static UnlimitedHeightmap * deSerialize(std::istream &istr);
};

#endif

