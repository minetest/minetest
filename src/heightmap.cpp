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

/*
(c) 2010 Perttu Ahola <celeron55@gmail.com>
*/

#include "heightmap.h"

/*
	ValueGenerator
*/

ValueGenerator* ValueGenerator::deSerialize(std::string line)
{
	std::istringstream ss(line);
	//ss.imbue(std::locale("C"));
	
	std::string name;
	std::getline(ss, name, ' ');

	if(name == "constant")
	{
		f32 value;
		ss>>value;
		
		return new ConstantGenerator(value);
	}
	else if(name == "linear")
	{
		f32 height;
		v2f slope;

		ss>>height;
		ss>>slope.X;
		ss>>slope.Y;

		return new LinearGenerator(height, slope);
	}
	else if(name == "power")
	{
		f32 height;
		v2f slope;
		f32 power;

		ss>>height;
		ss>>slope.X;
		ss>>slope.Y;
		ss>>power;

		return new PowerGenerator(height, slope, power);
	}
	else
	{
		throw SerializationError
		("Invalid heightmap generator (deSerialize)");
	}
}

/*
	FixedHeightmap
*/

f32 FixedHeightmap::avgNeighbours(v2s16 p, s16 d)
{
	v2s16 dirs[4] = {
		v2s16(1,0),
		v2s16(0,1),
		v2s16(-1,0),
		v2s16(0,-1)
	};
	f32 sum = 0.0;
	f32 count = 0.0;
	for(u16 i=0; i<4; i++){
		v2s16 p2 = p + dirs[i] * d;
		f32 n = getGroundHeightParent(p2);
		if(n < GROUNDHEIGHT_VALID_MINVALUE)
			continue;
		sum += n;
		count += 1.0;
	}
	assert(count > 0.001);
	return sum / count;
}

f32 FixedHeightmap::avgDiagNeighbours(v2s16 p, s16 d)
{
	v2s16 dirs[4] = {
		v2s16(1,1),
		v2s16(-1,-1),
		v2s16(-1,1),
		v2s16(1,-1)
	};
	f32 sum = 0.0;
	f32 count = 0.0;
	for(u16 i=0; i<4; i++){
		v2s16 p2 = p + dirs[i] * d;
		f32 n = getGroundHeightParent(p2);
		if(n < GROUNDHEIGHT_VALID_MINVALUE)
			continue;
		sum += n;
		count += 1.0;
	}
	assert(count > 0.001);
	return sum / count;
}

/*
	Adds a point to transform into a diamond pattern
	center = Center of the diamond phase (center of a square)
	a = Side length of the existing square (2, 4, 8, ...)

	Adds the center points of the next squares to next_squares as
	dummy "true" values.
*/
void FixedHeightmap::makeDiamond(
		v2s16 center,
		s16 a,
		f32 randmax,
		core::map<v2s16, bool> &next_squares)
{
	/*dstream<<"makeDiamond(): center="
			<<"("<<center.X<<","<<center.Y<<")"
			<<", a="<<a<<", randmax="<<randmax
			<<", next_squares.size()="<<next_squares.size()
			<<std::endl;*/

	f32 n = avgDiagNeighbours(center, a/2);
	// Add (-1.0...1.0) * randmax
	n += ((float)myrand() / (float)(MYRAND_MAX/2) - 1.0)*randmax;
	bool worked = setGroundHeightParent(center, n);
	
	if(a >= 2 && worked)
	{
		next_squares[center + a/2*v2s16(-1,0)] = true;
		next_squares[center + a/2*v2s16(1,0)] = true;
		next_squares[center + a/2*v2s16(0,-1)] = true;
		next_squares[center + a/2*v2s16(0,1)] = true;
	}
}

/*
	Adds a point to transform into a square pattern
	center = The point that is added. The center of a diamond.
	a = Diameter of the existing diamond. (2, 4, 8, 16, ...)

	Adds center points of the next diamonds to next_diamonds.
*/
void FixedHeightmap::makeSquare(
		v2s16 center,
		s16 a,
		f32 randmax,
		core::map<v2s16, bool> &next_diamonds)
{
	/*dstream<<"makeSquare(): center="
			<<"("<<center.X<<","<<center.Y<<")"
			<<", a="<<a<<", randmax="<<randmax
			<<", next_diamonds.size()="<<next_diamonds.size()
			<<std::endl;*/

	f32 n = avgNeighbours(center, a/2);
	// Add (-1.0...1.0) * randmax
	n += ((float)myrand() / (float)(MYRAND_MAX/2) - 1.0)*randmax;
	bool worked = setGroundHeightParent(center, n);
	
	if(a >= 4 && worked)
	{
		next_diamonds[center + a/4*v2s16(1,1)] = true;
		next_diamonds[center + a/4*v2s16(-1,1)] = true;
		next_diamonds[center + a/4*v2s16(-1,-1)] = true;
		next_diamonds[center + a/4*v2s16(1,-1)] = true;
	}
}

void FixedHeightmap::DiamondSquare(f32 randmax, f32 randfactor)
{
	u16 a;
	if(W < H)
		a = W-1;
	else
		a = H-1;
	
	// Check that a is a power of two
	if((a & (a-1)) != 0)
		throw;
	
	core::map<v2s16, bool> next_diamonds;
	core::map<v2s16, bool> next_squares;

	next_diamonds[v2s16(a/2, a/2)] = true;
	
	while(a >= 2)
	{
		next_squares.clear();
		
		for(core::map<v2s16, bool>::Iterator
				i = next_diamonds.getIterator();
				i.atEnd() == false; i++)
		{
			v2s16 p = i.getNode()->getKey();
			makeDiamond(p, a, randmax, next_squares);
		}

		//print();
		
		next_diamonds.clear();
		
		for(core::map<v2s16, bool>::Iterator
				i = next_squares.getIterator();
				i.atEnd() == false; i++)
		{
			v2s16 p = i.getNode()->getKey();
			makeSquare(p, a, randmax, next_diamonds);
		}

		//print();
		
		a /= 2;
		randmax *= randfactor;
	}
}

void FixedHeightmap::generateContinued(f32 randmax, f32 randfactor,
		f32 *corners)
{
	DSTACK(__FUNCTION_NAME);
	/*dstream<<"FixedHeightmap("<<m_pos_on_master.X
			<<","<<m_pos_on_master.Y
			<<")::generateContinued()"<<std::endl;*/

	// Works only with blocksize=2,4,8,16,32,64,...
	s16 a = m_blocksize;
	
	// Check that a is a power of two
	if((a & (a-1)) != 0)
		throw;
	
	// Overwrite with GROUNDHEIGHT_NOTFOUND_SETVALUE
	for(s16 y=0; y<=a; y++){
		for(s16 x=0; x<=a; x++){
			v2s16 p(x,y);
			setGroundHeight(p, GROUNDHEIGHT_NOTFOUND_SETVALUE);
		}
	}

	/*
		Seed borders from master heightmap
		NOTE: Does this actually have any effect on the output?
	*/
	struct SeedSpec
	{
		v2s16 neighbour_start;
		v2s16 heightmap_start;
		v2s16 dir;
	};

	SeedSpec seeds[4] =
	{
		{ // Z- edge on X-axis
			v2s16(0, -1), // neighbour_start
			v2s16(0, 0), // heightmap_start
			v2s16(1, 0) // dir
		},
		{ // Z+ edge on X-axis
			v2s16(0, m_blocksize),
			v2s16(0, m_blocksize),
			v2s16(1, 0)
		},
		{ // X- edge on Z-axis
			v2s16(-1, 0),
			v2s16(0, 0),
			v2s16(0, 1)
		},
		{ // X+ edge on Z-axis
			v2s16(m_blocksize, 0),
			v2s16(m_blocksize, 0),
			v2s16(0, 1)
		},
	};

	for(s16 i=0; i<4; i++){
		v2s16 npos = seeds[i].neighbour_start + m_pos_on_master * m_blocksize;
		v2s16 hpos = seeds[i].heightmap_start;
		for(s16 s=0; s<m_blocksize+1; s++){
			f32 h = m_master->getGroundHeight(npos, false);
			//dstream<<"h="<<h<<std::endl;
			if(h < GROUNDHEIGHT_VALID_MINVALUE)
				continue;
				//break;
			setGroundHeight(hpos, h);
			hpos += seeds[i].dir;
			npos += seeds[i].dir;
		}
	}
	
	/*dstream<<"borders seeded:"<<std::endl;
	print();*/

	/*
		Fill with corners[] (if not already set)
	*/
	v2s16 dirs[4] = {
		v2s16(0,0),
		v2s16(1,0),
		v2s16(1,1),
		v2s16(0,1),
	};
	for(u16 i=0; i<4; i++){
		v2s16 npos = dirs[i] * a;
		// Don't replace already seeded corners
		f32 h = getGroundHeight(npos);
		if(h > GROUNDHEIGHT_VALID_MINVALUE)
			continue;
		setGroundHeight(dirs[i] * a, corners[i]);
	}
	
	/*dstream<<"corners filled:"<<std::endl;
	print();*/

	DiamondSquare(randmax, randfactor);
}

u32 FixedHeightmap::serializedLength(u8 version, u16 blocksize)
{
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: FixedHeightmap format not supported");
	
	// Any version
	{
		/*// [0] s32 blocksize
		// [4] v2s16 pos_on_master
		// [8] s32 data[W*H] (W=H=blocksize+1)
		return 4 + 4 + (blocksize+1)*(blocksize+1)*4;*/

		// [8] s32 data[W*H] (W=H=blocksize+1)
		return (blocksize+1)*(blocksize+1)*4;
	}
}

u32 FixedHeightmap::serializedLength(u8 version)
{
	return serializedLength(version, m_blocksize);
}

void FixedHeightmap::serialize(u8 *dest, u8 version)
{
	//dstream<<"FixedHeightmap::serialize"<<std::endl;

	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: FixedHeightmap format not supported");
	
	// Any version
	{
		/*writeU32(&dest[0], m_blocksize);
		writeV2S16(&dest[4], m_pos_on_master);
		u32 nodecount = W*H;
		for(u32 i=0; i<nodecount; i++)
		{
			writeS32(&dest[8+i*4], (s32)(m_data[i]*1000.0));
		}*/

		u32 nodecount = W*H;
		for(u32 i=0; i<nodecount; i++)
		{
			writeS32(&dest[i*4], (s32)(m_data[i]*1000.0));
		}
	}
}

void FixedHeightmap::deSerialize(u8 *source, u8 version)
{
	/*dstream<<"FixedHeightmap::deSerialize m_blocksize="
			<<m_blocksize<<std::endl;*/

	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: FixedHeightmap format not supported");
	
	// Any version
	{
		u32 nodecount = (m_blocksize+1)*(m_blocksize+1);
		for(u32 i=0; i<nodecount; i++)
		{
			m_data[i] = ((f32)readS32(&source[i*4]))/1000.0;
		}

		/*printf("source[0,1,2,3]=%x,%x,%x,%x\n",
				(int)source[0]&0xff,
				(int)source[1]&0xff,
				(int)source[2]&0xff,
				(int)source[3]&0xff);
				
		dstream<<"m_data[0]="<<m_data[0]<<", "
				<<"readS32(&source[0])="<<readS32(&source[0])
				<<std::endl;
		dstream<<"m_data[4*4]="<<m_data[4*4]<<", "
				<<"readS32(&source[4*4])="<<readS32(&source[4*4])
				<<std::endl;*/
	}
}


void setcolor(f32 h, f32 rangemin, f32 rangemax)
{
#ifndef _WIN32
	const char *colors[] =
	{
		"\x1b[40m",
		"\x1b[44m",
		"\x1b[46m",
		"\x1b[42m",
		"\x1b[43m",
		"\x1b[41m",
	};
	u16 colorcount = sizeof(colors)/sizeof(colors[0]);
	f32 scaled = (h - rangemin) / (rangemax - rangemin);
	u8 color = scaled * colorcount;
	if(color > colorcount-1)
		color = colorcount-1;
	/*printf("rangemin=%f, rangemax=%f, h=%f -> color=%i\n",
		rangemin,
		rangemax,
		h,
		color);*/
	printf("%s", colors[color]);
	//printf("\x1b[31;40m");
	//printf("\x1b[44;1m");
#endif
}
void resetcolor()
{
#ifndef _WIN32
	printf("\x1b[0m");
#endif
}

/*
	UnlimitedHeightmap
*/

void UnlimitedHeightmap::print()
{
	s16 minx =  10000;
	s16 miny =  10000;
	s16 maxx = -10000;
	s16 maxy = -10000;
	core::map<v2s16, FixedHeightmap*>::Iterator i;
	i = m_heightmaps.getIterator();
	if(i.atEnd()){
		printf("UnlimitedHeightmap::print(): empty.\n");
		return;
	}
	for(; i.atEnd() == false; i++)
	{
		v2s16 p = i.getNode()->getValue()->getPosOnMaster();
		if(p.X < minx) minx = p.X;
		if(p.Y < miny) miny = p.Y;
		if(p.X > maxx) maxx = p.X;
		if(p.Y > maxy) maxy = p.Y;
	}
	minx = minx * m_blocksize;
	miny = miny * m_blocksize;
	maxx = (maxx+1) * m_blocksize;
	maxy = (maxy+1) * m_blocksize;
	printf("UnlimitedHeightmap::print(): from (%i,%i) to (%i,%i)\n",
			minx, miny, maxx, maxy);
	
	// Calculate range
	f32 rangemin = 1e10;
	f32 rangemax = -1e10;
	for(s32 y=miny; y<=maxy; y++){
		for(s32 x=minx; x<=maxx; x++){
			f32 h = getGroundHeight(v2s16(x,y), false);
			if(h < GROUNDHEIGHT_VALID_MINVALUE)
				continue;
			if(h < rangemin)
				rangemin = h;
			if(h > rangemax)
				rangemax = h;
		}
	}

	printf("     ");
	for(s32 x=minx; x<=maxx; x++){
		printf("% .3d ", x);
	}
	printf("\n");
	
	for(s32 y=miny; y<=maxy; y++){
		printf("% .3d ", y);
		for(s32 x=minx; x<=maxx; x++){
			f32 n = getGroundHeight(v2s16(x,y), false);
			if(n < GROUNDHEIGHT_VALID_MINVALUE)
				printf("  -   ");
			else
			{
				setcolor(n, rangemin, rangemax);
				printf("% -5.1f", getGroundHeight(v2s16(x,y), false));
				resetcolor();
			}
		}
		printf("\n");
	}
}
	
FixedHeightmap * UnlimitedHeightmap::getHeightmap(v2s16 p_from, bool generate)
{
	DSTACK("UnlimitedHeightmap::getHeightmap()");
	/*
		We want to check that all neighbours of the wanted heightmap
		exist.
		This is because generating the neighboring heightmaps will
		modify the current one.
	*/
	
	if(generate)
	{
		// Go through all neighbors (corners also) and the current one
		// and generate every one of them.
		for(s16 x=p_from.X-1; x<=p_from.X+1; x++)
		for(s16 y=p_from.Y-1; y<=p_from.Y+1; y++)
		{
			v2s16 p(x,y);

			// Check if exists
			core::map<v2s16, FixedHeightmap*>::Node *n = m_heightmaps.find(p);
			if(n != NULL)
				continue;
			
			// Doesn't exist
			// Generate it

			FixedHeightmap *heightmap = new FixedHeightmap(this, p, m_blocksize);

			m_heightmaps.insert(p, heightmap);
			
			f32 corners[4] = {
				m_base_generator->getValue(p+v2s16(0,0)),
				m_base_generator->getValue(p+v2s16(1,0)),
				m_base_generator->getValue(p+v2s16(1,1)),
				m_base_generator->getValue(p+v2s16(0,1)),
			};

			f32 randmax = m_randmax_generator->getValue(p);
			f32 randfactor = m_randfactor_generator->getValue(p);

			heightmap->generateContinued(randmax, randfactor, corners);
		}
	}

	core::map<v2s16, FixedHeightmap*>::Node *n = m_heightmaps.find(p_from);

	if(n != NULL)
	{
		return n->getValue();
	}
	else
	{
		throw InvalidPositionException
		("Something went really wrong in UnlimitedHeightmap::getHeightmap");
	}
}

f32 UnlimitedHeightmap::getGroundHeight(v2s16 p, bool generate)
{
	v2s16 heightmappos = getNodeHeightmapPos(p);
	v2s16 relpos = p - heightmappos*m_blocksize;
	try{
		FixedHeightmap * href = getHeightmap(heightmappos, generate);
		f32 h = href->getGroundHeight(relpos);
		if(h > GROUNDHEIGHT_VALID_MINVALUE)
			return h;
	}
	catch(InvalidPositionException){}
	/*
		If on border or in the (0,0) corner, try to get from
		overlapping heightmaps
	*/
	if(relpos.X == 0){
		try{
			FixedHeightmap * href = getHeightmap(
					heightmappos-v2s16(1,0), false);
			f32 h = href->getGroundHeight(v2s16(m_blocksize, relpos.Y));
			if(h > GROUNDHEIGHT_VALID_MINVALUE)
				return h;
		}
		catch(InvalidPositionException){}
	}
	if(relpos.Y == 0){
		try{
			FixedHeightmap * href = getHeightmap(
					heightmappos-v2s16(0,1), false);
			f32 h = href->getGroundHeight(v2s16(relpos.X, m_blocksize));
			if(h > GROUNDHEIGHT_VALID_MINVALUE)
				return h;
		}
		catch(InvalidPositionException){}
	}
	if(relpos.X == 0 && relpos.Y == 0){
		try{
			FixedHeightmap * href = getHeightmap(
					heightmappos-v2s16(1,1), false);
			f32 h = href->getGroundHeight(v2s16(m_blocksize, m_blocksize));
			if(h > GROUNDHEIGHT_VALID_MINVALUE)
				return h;
		}
		catch(InvalidPositionException){}
	}
	return GROUNDHEIGHT_NOTFOUND_SETVALUE;
}

void UnlimitedHeightmap::setGroundHeight(v2s16 p, f32 y, bool generate)
{
	bool was_set = false;

	v2s16 heightmappos = getNodeHeightmapPos(p);
	v2s16 relpos = p - heightmappos*m_blocksize;
	/*dstream<<"UnlimitedHeightmap::setGroundHeight(("
			<<p.X<<","<<p.Y<<"), "<<y<<"): "
			<<"heightmappos=("<<heightmappos.X<<","
			<<heightmappos.Y<<") relpos=("
			<<relpos.X<<","<<relpos.Y<<")"
			<<std::endl;*/
	try{
		FixedHeightmap * href = getHeightmap(heightmappos, generate);
		href->setGroundHeight(relpos, y);
		was_set = true;
	}catch(InvalidPositionException){}
	// Update in neighbour heightmap if it's at border
	if(relpos.X == 0){
		try{
			FixedHeightmap * href = getHeightmap(
					heightmappos-v2s16(1,0), generate);
			href->setGroundHeight(v2s16(m_blocksize, relpos.Y), y);
			was_set = true;
		}catch(InvalidPositionException){}
	}
	if(relpos.Y == 0){
		try{
			FixedHeightmap * href = getHeightmap(
					heightmappos-v2s16(0,1), generate);
			href->setGroundHeight(v2s16(relpos.X, m_blocksize), y);
			was_set = true;
		}catch(InvalidPositionException){}
	}
	if(relpos.X == 0 && relpos.Y == 0){
		try{
			FixedHeightmap * href = getHeightmap(
					heightmappos-v2s16(1,1), generate);
			href->setGroundHeight(v2s16(m_blocksize, m_blocksize), y);
			was_set = true;
		}catch(InvalidPositionException){}
	}

	if(was_set == false)
	{
		throw InvalidPositionException
				("UnlimitedHeightmap failed to set height");
	}
}


void UnlimitedHeightmap::serialize(std::ostream &os, u8 version)
{
	//dstream<<"UnlimitedHeightmap::serialize()"<<std::endl;

	if(!ser_ver_supported(version))
		throw VersionMismatchException
		("ERROR: UnlimitedHeightmap format not supported");
	
	if(version <= 7)
	{
		/*if(m_base_generator->getId() != VALUE_GENERATOR_ID_CONSTANT
		|| m_randmax_generator->getId() != VALUE_GENERATOR_ID_CONSTANT
		|| m_randfactor_generator->getId() != VALUE_GENERATOR_ID_CONSTANT)*/
		if(std::string(m_base_generator->getName()) != "constant"
		|| std::string(m_randmax_generator->getName()) != "constant"
		|| std::string(m_randfactor_generator->getName()) != "constant")
		{
			throw SerializationError
			("Cannot write UnlimitedHeightmap in old version: "
			"Generators are not ConstantGenerators.");
		}

		f32 basevalue = ((ConstantGenerator*)m_base_generator)->m_value;
		f32 randmax = ((ConstantGenerator*)m_randmax_generator)->m_value;
		f32 randfactor = ((ConstantGenerator*)m_randfactor_generator)->m_value;

		// Write version
		os.write((char*)&version, 1);
		
		/*
			[0] u16 blocksize
			[2] s32 randmax*1000
			[6] s32 randfactor*1000
			[10] s32 basevalue*1000
			[14] u32 heightmap_count
			[18] X * (v2s16 pos + heightmap)
		*/
		u32 heightmap_size =
				FixedHeightmap::serializedLength(version, m_blocksize);
		u32 heightmap_count = m_heightmaps.size();

		//dstream<<"heightmap_size="<<heightmap_size<<std::endl;

		u32 datasize = 2+4+4+4+4+heightmap_count*(4+heightmap_size);
		SharedBuffer<u8> data(datasize);
		
		writeU16(&data[0], m_blocksize);
		writeU32(&data[2], (s32)(randmax*1000.0));
		writeU32(&data[6], (s32)(randfactor*1000.0));
		writeU32(&data[10], (s32)(basevalue*1000.0));
		writeU32(&data[14], heightmap_count);

		core::map<v2s16, FixedHeightmap*>::Iterator j;
		j = m_heightmaps.getIterator();
		u32 i=0;
		for(; j.atEnd() == false; j++)
		{
			FixedHeightmap *hm = j.getNode()->getValue();
			v2s16 pos = j.getNode()->getKey();
			writeV2S16(&data[18+i*(4+heightmap_size)], pos);
			hm->serialize(&data[18+i*(4+heightmap_size)+4], version);
			i++;
		}

		os.write((char*)*data, data.getSize());
	}
	else
	{
		// Write version
		os.write((char*)&version, 1);
		
		u8 buf[4];
		
		writeU16(buf, m_blocksize);
		os.write((char*)buf, 2);

		/*m_randmax_generator->serialize(os, version);
		m_randfactor_generator->serialize(os, version);
		m_base_generator->serialize(os, version);*/
		m_randmax_generator->serialize(os);
		m_randfactor_generator->serialize(os);
		m_base_generator->serialize(os);

		u32 heightmap_count = m_heightmaps.size();
		writeU32(buf, heightmap_count);
		os.write((char*)buf, 4);

		u32 heightmap_size =
				FixedHeightmap::serializedLength(version, m_blocksize);

		SharedBuffer<u8> hmdata(heightmap_size);

		core::map<v2s16, FixedHeightmap*>::Iterator j;
		j = m_heightmaps.getIterator();
		u32 i=0;
		for(; j.atEnd() == false; j++)
		{
			v2s16 pos = j.getNode()->getKey();
			writeV2S16(buf, pos);
			os.write((char*)buf, 4);

			FixedHeightmap *hm = j.getNode()->getValue();
			hm->serialize(*hmdata, version);
			os.write((char*)*hmdata, hmdata.getSize());

			i++;
		}
	}
}

UnlimitedHeightmap * UnlimitedHeightmap::deSerialize(std::istream &is)
{
	u8 version;
	is.read((char*)&version, 1);
	
	if(!ser_ver_supported(version))
		throw VersionMismatchException("ERROR: UnlimitedHeightmap format not supported");
	
	if(version <= 7)
	{
		/*
			[0] u16 blocksize
			[2] s32 randmax*1000
			[6] s32 randfactor*1000
			[10] s32 basevalue*1000
			[14] u32 heightmap_count
			[18] X * (v2s16 pos + heightmap)
		*/
		SharedBuffer<u8> data(18);
		is.read((char*)*data, 18);
		if(is.gcount() != 18)
			throw SerializationError
					("UnlimitedHeightmap::deSerialize: no enough input data");
		s16 blocksize = readU16(&data[0]);
		f32 randmax = (f32)readU32(&data[2]) / 1000.0;
		f32 randfactor = (f32)readU32(&data[6]) / 1000.0;
		f32 basevalue = (f32)readU32(&data[10]) / 1000.0;
		u32 heightmap_count = readU32(&data[14]);

		/*dstream<<"UnlimitedHeightmap::deSerialize():"
				<<" blocksize="<<blocksize
				<<" heightmap_count="<<heightmap_count
				<<std::endl;*/

		u32 heightmap_size =
				FixedHeightmap::serializedLength(version, blocksize);

		//dstream<<"heightmap_size="<<heightmap_size<<std::endl;

		ValueGenerator *maxgen = new ConstantGenerator(randmax);
		ValueGenerator *factorgen = new ConstantGenerator(randfactor);
		ValueGenerator *basegen = new ConstantGenerator(basevalue);

		UnlimitedHeightmap *hm = new UnlimitedHeightmap
				(blocksize, maxgen, factorgen, basegen);

		for(u32 i=0; i<heightmap_count; i++)
		{
			//dstream<<"i="<<i<<std::endl;
			SharedBuffer<u8> data(4+heightmap_size);
			is.read((char*)*data, 4+heightmap_size);
			if(is.gcount() != (s32)(4+heightmap_size)){
				delete hm;
				throw SerializationError
						("UnlimitedHeightmap::deSerialize: no enough input data");
			}
			v2s16 pos = readV2S16(&data[0]);
			FixedHeightmap *f = new FixedHeightmap(hm, pos, blocksize);
			f->deSerialize(&data[4], version);
			hm->m_heightmaps.insert(pos, f);
		}
		return hm;
	}
	else
	{
		u8 buf[4];
		
		is.read((char*)buf, 2);
		s16 blocksize = readU16(buf);

		ValueGenerator *maxgen = ValueGenerator::deSerialize(is);
		ValueGenerator *factorgen = ValueGenerator::deSerialize(is);
		ValueGenerator *basegen = ValueGenerator::deSerialize(is);

		is.read((char*)buf, 4);
		u32 heightmap_count = readU32(buf);

		u32 heightmap_size =
				FixedHeightmap::serializedLength(version, blocksize);

		UnlimitedHeightmap *hm = new UnlimitedHeightmap
				(blocksize, maxgen, factorgen, basegen);

		for(u32 i=0; i<heightmap_count; i++)
		{
			is.read((char*)buf, 4);
			v2s16 pos = readV2S16(buf);

			SharedBuffer<u8> data(heightmap_size);
			is.read((char*)*data, heightmap_size);
			if(is.gcount() != (s32)(heightmap_size)){
				delete hm;
				throw SerializationError
						("UnlimitedHeightmap::deSerialize: no enough input data");
			}
			FixedHeightmap *f = new FixedHeightmap(hm, pos, blocksize);
			f->deSerialize(*data, version);
			hm->m_heightmaps.insert(pos, f);
		}
		return hm;
	}
}


