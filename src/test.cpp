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

#include "test.h"
#include "common_irrlicht.h"
#include "debug.h"
#include "map.h"
#include "player.h"
#include "main.h"
#include "socket.h"
#include "connection.h"
#include "utility.h"
#include "serialization.h"
#include "voxel.h"
#include <sstream>
#include "porting.h"

/*
	Asserts that the exception occurs
*/
#define EXCEPTION_CHECK(EType, code)\
{\
	bool exception_thrown = false;\
	try{ code; }\
	catch(EType &e) { exception_thrown = true; }\
	assert(exception_thrown);\
}

struct TestUtilities
{
	void Run()
	{
		/*dstream<<"wrapDegrees(100.0) = "<<wrapDegrees(100.0)<<std::endl;
		dstream<<"wrapDegrees(720.5) = "<<wrapDegrees(720.5)<<std::endl;
		dstream<<"wrapDegrees(-0.5) = "<<wrapDegrees(-0.5)<<std::endl;*/
		assert(fabs(wrapDegrees(100.0) - 100.0) < 0.001);
		assert(fabs(wrapDegrees(720.5) - 0.5) < 0.001);
		assert(fabs(wrapDegrees(-0.5) - (-0.5)) < 0.001);
		assert(fabs(wrapDegrees(-365.5) - (-5.5)) < 0.001);
		assert(lowercase("Foo bAR") == "foo bar");
		assert(is_yes("YeS") == true);
		assert(is_yes("") == false);
		assert(is_yes("FAlse") == false);
	}
};
		
struct TestCompress
{
	void Run()
	{
		{ // ver 0

		SharedBuffer<u8> fromdata(4);
		fromdata[0]=1;
		fromdata[1]=5;
		fromdata[2]=5;
		fromdata[3]=1;
		
		std::ostringstream os(std::ios_base::binary);
		compress(fromdata, os, 0);

		std::string str_out = os.str();
		
		dstream<<"str_out.size()="<<str_out.size()<<std::endl;
		dstream<<"TestCompress: 1,5,5,1 -> ";
		for(u32 i=0; i<str_out.size(); i++)
		{
			dstream<<(u32)str_out[i]<<",";
		}
		dstream<<std::endl;

		assert(str_out.size() == 10);

		assert(str_out[0] == 0);
		assert(str_out[1] == 0);
		assert(str_out[2] == 0);
		assert(str_out[3] == 4);
		assert(str_out[4] == 0);
		assert(str_out[5] == 1);
		assert(str_out[6] == 1);
		assert(str_out[7] == 5);
		assert(str_out[8] == 0);
		assert(str_out[9] == 1);

		std::istringstream is(str_out, std::ios_base::binary);
		std::ostringstream os2(std::ios_base::binary);

		decompress(is, os2, 0);
		std::string str_out2 = os2.str();

		dstream<<"decompress: ";
		for(u32 i=0; i<str_out2.size(); i++)
		{
			dstream<<(u32)str_out2[i]<<",";
		}
		dstream<<std::endl;

		assert(str_out2.size() == fromdata.getSize());

		for(u32 i=0; i<str_out2.size(); i++)
		{
			assert(str_out2[i] == fromdata[i]);
		}

		}

		{ // ver HIGHEST

		SharedBuffer<u8> fromdata(4);
		fromdata[0]=1;
		fromdata[1]=5;
		fromdata[2]=5;
		fromdata[3]=1;
		
		std::ostringstream os(std::ios_base::binary);
		compress(fromdata, os, SER_FMT_VER_HIGHEST);

		std::string str_out = os.str();
		
		dstream<<"str_out.size()="<<str_out.size()<<std::endl;
		dstream<<"TestCompress: 1,5,5,1 -> ";
		for(u32 i=0; i<str_out.size(); i++)
		{
			dstream<<(u32)str_out[i]<<",";
		}
		dstream<<std::endl;

		/*assert(str_out.size() == 10);

		assert(str_out[0] == 0);
		assert(str_out[1] == 0);
		assert(str_out[2] == 0);
		assert(str_out[3] == 4);
		assert(str_out[4] == 0);
		assert(str_out[5] == 1);
		assert(str_out[6] == 1);
		assert(str_out[7] == 5);
		assert(str_out[8] == 0);
		assert(str_out[9] == 1);*/

		std::istringstream is(str_out, std::ios_base::binary);
		std::ostringstream os2(std::ios_base::binary);

		decompress(is, os2, SER_FMT_VER_HIGHEST);
		std::string str_out2 = os2.str();

		dstream<<"decompress: ";
		for(u32 i=0; i<str_out2.size(); i++)
		{
			dstream<<(u32)str_out2[i]<<",";
		}
		dstream<<std::endl;

		assert(str_out2.size() == fromdata.getSize());

		for(u32 i=0; i<str_out2.size(); i++)
		{
			assert(str_out2[i] == fromdata[i]);
		}

		}
	}
};

struct TestMapNode
{
	void Run()
	{
		MapNode n;

		// Default values
		assert(n.d == CONTENT_AIR);
		assert(n.getLight(LIGHTBANK_DAY) == 0);
		assert(n.getLight(LIGHTBANK_NIGHT) == 0);
		
		// Transparency
		n.d = CONTENT_AIR;
		assert(n.light_propagates() == true);
		n.d = CONTENT_STONE;
		assert(n.light_propagates() == false);
	}
};

struct TestVoxelManipulator
{
	void Run()
	{
		/*
			VoxelArea
		*/

		VoxelArea a(v3s16(-1,-1,-1), v3s16(1,1,1));
		assert(a.index(0,0,0) == 1*3*3 + 1*3 + 1);
		assert(a.index(-1,-1,-1) == 0);
		
		VoxelArea c(v3s16(-2,-2,-2), v3s16(2,2,2));
		// An area that is 1 bigger in x+ and z-
		VoxelArea d(v3s16(-2,-2,-3), v3s16(3,2,2));
		
		core::list<VoxelArea> aa;
		d.diff(c, aa);
		
		// Correct results
		core::array<VoxelArea> results;
		results.push_back(VoxelArea(v3s16(-2,-2,-3),v3s16(3,2,-3)));
		results.push_back(VoxelArea(v3s16(3,-2,-2),v3s16(3,2,2)));

		assert(aa.size() == results.size());
		
		dstream<<"Result of diff:"<<std::endl;
		for(core::list<VoxelArea>::Iterator
				i = aa.begin(); i != aa.end(); i++)
		{
			i->print(dstream);
			dstream<<std::endl;
			
			s32 j = results.linear_search(*i);
			assert(j != -1);
			results.erase(j, 1);
		}


		/*
			VoxelManipulator
		*/
		
		VoxelManipulator v;

		v.print(dstream);

		dstream<<"*** Setting (-1,0,-1)=2 ***"<<std::endl;
		
		v.setNodeNoRef(v3s16(-1,0,-1), MapNode(2));

		v.print(dstream);

 		assert(v.getNode(v3s16(-1,0,-1)).d == 2);

		dstream<<"*** Reading from inexistent (0,0,-1) ***"<<std::endl;

		EXCEPTION_CHECK(InvalidPositionException, v.getNode(v3s16(0,0,-1)));

		v.print(dstream);

		dstream<<"*** Adding area ***"<<std::endl;

		v.addArea(a);
		
		v.print(dstream);

		assert(v.getNode(v3s16(-1,0,-1)).d == 2);
		EXCEPTION_CHECK(InvalidPositionException, v.getNode(v3s16(0,1,1)));

#if 0
		/*
			Water stuff
		*/

		v.clear();

		const char *content =
			"#...######  "
			"#...##..##  "
			"#........ .."
			"############"

			"#...######  "
			"#...##..##  "
			"#........#  "
			"############"
		;

		v3s16 size(12, 4, 2);
		VoxelArea area(v3s16(0,0,0), size-v3s16(1,1,1));
		
		const char *p = content;
		for(s16 z=0; z<size.Z; z++)
		for(s16 y=size.Y-1; y>=0; y--)
		for(s16 x=0; x<size.X; x++)
		{
			MapNode n;
			//n.pressure = size.Y - y;
			if(*p == '#')
				n.d = CONTENT_STONE;
			else if(*p == '.')
				n.d = CONTENT_WATER;
			else if(*p == ' ')
				n.d = CONTENT_AIR;
			else
				assert(0);
			v.setNode(v3s16(x,y,z), n);
			p++;
		}

		v.print(dstream, VOXELPRINT_WATERPRESSURE);
		
		core::map<v3s16, u8> active_nodes;
		v.updateAreaWaterPressure(area, active_nodes);

		v.print(dstream, VOXELPRINT_WATERPRESSURE);
		
		//s16 highest_y = -32768;
		/*
			NOTE: These are commented out because this behaviour is changed
			      all the time
		*/
		//assert(v.getWaterPressure(v3s16(7, 1, 1), highest_y, 0) == -1);
		//assert(highest_y == 3);
		/*assert(v.getWaterPressure(v3s16(7, 1, 1), highest_y, 0) == 3);
		//assert(highest_y == 3);*/
		
		active_nodes.clear();
		active_nodes[v3s16(9,1,0)] = 1;
		//v.flowWater(active_nodes, 0, true, 1000);
		v.flowWater(active_nodes, 0, false, 1000);
		
		dstream<<"Final result of flowWater:"<<std::endl;
		v.print(dstream, VOXELPRINT_WATERPRESSURE);
#endif
		
		//assert(0);
	}
};

struct TestMapBlock
{
	class TC : public NodeContainer
	{
	public:

		MapNode node;
		bool position_valid;
		core::list<v3s16> validity_exceptions;

		TC()
		{
			position_valid = true;
		}

		virtual bool isValidPosition(v3s16 p)
		{
			//return position_valid ^ (p==position_valid_exception);
			bool exception = false;
			for(core::list<v3s16>::Iterator i=validity_exceptions.begin();
					i != validity_exceptions.end(); i++)
			{
				if(p == *i)
				{
					exception = true;
					break;
				}
			}
			return exception ? !position_valid : position_valid;
		}

		virtual MapNode getNode(v3s16 p)
		{
			if(isValidPosition(p) == false)
				throw InvalidPositionException();
			return node;
		}

		virtual void setNode(v3s16 p, MapNode & n)
		{
			if(isValidPosition(p) == false)
				throw InvalidPositionException();
		};

		virtual u16 nodeContainerId() const
		{
			return 666;
		}
	};

	void Run()
	{
		TC parent;
		
		MapBlock b(&parent, v3s16(1,1,1));
		v3s16 relpos(MAP_BLOCKSIZE, MAP_BLOCKSIZE, MAP_BLOCKSIZE);

		assert(b.getPosRelative() == relpos);

		assert(b.getBox().MinEdge.X == MAP_BLOCKSIZE);
		assert(b.getBox().MaxEdge.X == MAP_BLOCKSIZE*2-1);
		assert(b.getBox().MinEdge.Y == MAP_BLOCKSIZE);
		assert(b.getBox().MaxEdge.Y == MAP_BLOCKSIZE*2-1);
		assert(b.getBox().MinEdge.Z == MAP_BLOCKSIZE);
		assert(b.getBox().MaxEdge.Z == MAP_BLOCKSIZE*2-1);
		
		assert(b.isValidPosition(v3s16(0,0,0)) == true);
		assert(b.isValidPosition(v3s16(-1,0,0)) == false);
		assert(b.isValidPosition(v3s16(-1,-142,-2341)) == false);
		assert(b.isValidPosition(v3s16(-124,142,2341)) == false);
		assert(b.isValidPosition(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1)) == true);
		assert(b.isValidPosition(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE,MAP_BLOCKSIZE-1)) == false);

		/*
			TODO: this method should probably be removed
			if the block size isn't going to be set variable
		*/
		/*assert(b.getSizeNodes() == v3s16(MAP_BLOCKSIZE,
				MAP_BLOCKSIZE, MAP_BLOCKSIZE));*/
		
		// Changed flag should be initially set
		assert(b.getChangedFlag() == true);
		b.resetChangedFlag();
		assert(b.getChangedFlag() == false);

		// All nodes should have been set to
		// .d=CONTENT_AIR and .getLight() = 0
		for(u16 z=0; z<MAP_BLOCKSIZE; z++)
		for(u16 y=0; y<MAP_BLOCKSIZE; y++)
		for(u16 x=0; x<MAP_BLOCKSIZE; x++)
		{
			assert(b.getNode(v3s16(x,y,z)).d == CONTENT_AIR);
			assert(b.getNode(v3s16(x,y,z)).getLight(LIGHTBANK_DAY) == 0);
			assert(b.getNode(v3s16(x,y,z)).getLight(LIGHTBANK_NIGHT) == 0);
		}
		
		/*
			Parent fetch functions
		*/
		parent.position_valid = false;
		parent.node.d = 5;

		MapNode n;
		
		// Positions in the block should still be valid
		assert(b.isValidPositionParent(v3s16(0,0,0)) == true);
		assert(b.isValidPositionParent(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1)) == true);
		n = b.getNodeParent(v3s16(0,MAP_BLOCKSIZE-1,0));
		assert(n.d == CONTENT_AIR);

		// ...but outside the block they should be invalid
		assert(b.isValidPositionParent(v3s16(-121,2341,0)) == false);
		assert(b.isValidPositionParent(v3s16(-1,0,0)) == false);
		assert(b.isValidPositionParent(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1,MAP_BLOCKSIZE)) == false);
		
		{
			bool exception_thrown = false;
			try{
				// This should throw an exception
				MapNode n = b.getNodeParent(v3s16(0,0,-1));
			}
			catch(InvalidPositionException &e)
			{
				exception_thrown = true;
			}
			assert(exception_thrown);
		}

		parent.position_valid = true;
		// Now the positions outside should be valid
		assert(b.isValidPositionParent(v3s16(-121,2341,0)) == true);
		assert(b.isValidPositionParent(v3s16(-1,0,0)) == true);
		assert(b.isValidPositionParent(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1,MAP_BLOCKSIZE)) == true);
		n = b.getNodeParent(v3s16(0,0,MAP_BLOCKSIZE));
		assert(n.d == 5);

		/*
			Set a node
		*/
		v3s16 p(1,2,0);
		n.d = 4;
		b.setNode(p, n);
		assert(b.getNode(p).d == 4);
		//TODO: Update to new system
		/*assert(b.getNodeTile(p) == 4);
		assert(b.getNodeTile(v3s16(-1,-1,0)) == 5);*/
		
		/*
			propagateSunlight()
		*/
		// Set lighting of all nodes to 0
		for(u16 z=0; z<MAP_BLOCKSIZE; z++){
			for(u16 y=0; y<MAP_BLOCKSIZE; y++){
				for(u16 x=0; x<MAP_BLOCKSIZE; x++){
					MapNode n = b.getNode(v3s16(x,y,z));
					n.setLight(LIGHTBANK_DAY, 0);
					n.setLight(LIGHTBANK_NIGHT, 0);
					b.setNode(v3s16(x,y,z), n);
				}
			}
		}
		{
			/*
				Check how the block handles being a lonely sky block
			*/
			parent.position_valid = true;
			b.setIsUnderground(false);
			parent.node.d = CONTENT_AIR;
			parent.node.setLight(LIGHTBANK_DAY, LIGHT_SUN);
			parent.node.setLight(LIGHTBANK_NIGHT, 0);
			core::map<v3s16, bool> light_sources;
			// The bottom block is invalid, because we have a shadowing node
			assert(b.propagateSunlight(light_sources) == false);
			assert(b.getNode(v3s16(1,4,0)).getLight(LIGHTBANK_DAY) == LIGHT_SUN);
			assert(b.getNode(v3s16(1,3,0)).getLight(LIGHTBANK_DAY) == LIGHT_SUN);
			assert(b.getNode(v3s16(1,2,0)).getLight(LIGHTBANK_DAY) == 0);
			assert(b.getNode(v3s16(1,1,0)).getLight(LIGHTBANK_DAY) == 0);
			assert(b.getNode(v3s16(1,0,0)).getLight(LIGHTBANK_DAY) == 0);
			assert(b.getNode(v3s16(1,2,3)).getLight(LIGHTBANK_DAY) == LIGHT_SUN);
			assert(b.getFaceLight2(1000, p, v3s16(0,1,0)) == LIGHT_SUN);
			assert(b.getFaceLight2(1000, p, v3s16(0,-1,0)) == 0);
			assert(b.getFaceLight2(0, p, v3s16(0,-1,0)) == 0);
			// According to MapBlock::getFaceLight,
			// The face on the z+ side should have double-diminished light
			//assert(b.getFaceLight(p, v3s16(0,0,1)) == diminish_light(diminish_light(LIGHT_MAX)));
			// The face on the z+ side should have diminished light
			assert(b.getFaceLight2(1000, p, v3s16(0,0,1)) == diminish_light(LIGHT_MAX));
		}
		/*
			Check how the block handles being in between blocks with some non-sunlight
			while being underground
		*/
		{
			// Make neighbours to exist and set some non-sunlight to them
			parent.position_valid = true;
			b.setIsUnderground(true);
			parent.node.setLight(LIGHTBANK_DAY, LIGHT_MAX/2);
			core::map<v3s16, bool> light_sources;
			// The block below should be valid because there shouldn't be
			// sunlight in there either
			assert(b.propagateSunlight(light_sources, true) == true);
			// Should not touch nodes that are not affected (that is, all of them)
			//assert(b.getNode(v3s16(1,2,3)).getLight() == LIGHT_SUN);
			// Should set light of non-sunlighted blocks to 0.
			assert(b.getNode(v3s16(1,2,3)).getLight(LIGHTBANK_DAY) == 0);
		}
		/*
			Set up a situation where:
			- There is only air in this block
			- There is a valid non-sunlighted block at the bottom, and
			- Invalid blocks elsewhere.
			- the block is not underground.

			This should result in bottom block invalidity
		*/
		{
			b.setIsUnderground(false);
			// Clear block
			for(u16 z=0; z<MAP_BLOCKSIZE; z++){
				for(u16 y=0; y<MAP_BLOCKSIZE; y++){
					for(u16 x=0; x<MAP_BLOCKSIZE; x++){
						MapNode n;
						n.d = CONTENT_AIR;
						n.setLight(LIGHTBANK_DAY, 0);
						b.setNode(v3s16(x,y,z), n);
					}
				}
			}
			// Make neighbours invalid
			parent.position_valid = false;
			// Add exceptions to the top of the bottom block
			for(u16 x=0; x<MAP_BLOCKSIZE; x++)
			for(u16 z=0; z<MAP_BLOCKSIZE; z++)
			{
				parent.validity_exceptions.push_back(v3s16(MAP_BLOCKSIZE+x, MAP_BLOCKSIZE-1, MAP_BLOCKSIZE+z));
			}
			// Lighting value for the valid nodes
			parent.node.setLight(LIGHTBANK_DAY, LIGHT_MAX/2);
			core::map<v3s16, bool> light_sources;
			// Bottom block is not valid
			assert(b.propagateSunlight(light_sources) == false);
		}
	}
};

struct TestMapSector
{
	class TC : public NodeContainer
	{
	public:

		MapNode node;
		bool position_valid;

		TC()
		{
			position_valid = true;
		}

		virtual bool isValidPosition(v3s16 p)
		{
			return position_valid;
		}

		virtual MapNode getNode(v3s16 p)
		{
			if(position_valid == false)
				throw InvalidPositionException();
			return node;
		}

		virtual void setNode(v3s16 p, MapNode & n)
		{
			if(position_valid == false)
				throw InvalidPositionException();
		};
		
		virtual u16 nodeContainerId() const
		{
			return 666;
		}
	};
	
	void Run()
	{
		TC parent;
		parent.position_valid = false;
		
		// Create one with no heightmaps
		ServerMapSector sector(&parent, v2s16(1,1));
		
		EXCEPTION_CHECK(InvalidPositionException, sector.getBlockNoCreate(0));
		EXCEPTION_CHECK(InvalidPositionException, sector.getBlockNoCreate(1));

		MapBlock * bref = sector.createBlankBlock(-2);
		
		EXCEPTION_CHECK(InvalidPositionException, sector.getBlockNoCreate(0));
		assert(sector.getBlockNoCreate(-2) == bref);
		
		//TODO: Check for AlreadyExistsException

		/*bool exception_thrown = false;
		try{
			sector.getBlock(0);
		}
		catch(InvalidPositionException &e){
			exception_thrown = true;
		}
		assert(exception_thrown);*/

	}
};

struct TestSocket
{
	void Run()
	{
		const int port = 30003;
		UDPSocket socket;
		socket.Bind(port);

		const char sendbuffer[] = "hello world!";
		socket.Send(Address(127,0,0,1,port), sendbuffer, sizeof(sendbuffer));

		sleep_ms(50);

		char rcvbuffer[256];
		memset(rcvbuffer, 0, sizeof(rcvbuffer));
		Address sender;
		for(;;)
		{
			int bytes_read = socket.Receive(sender, rcvbuffer, sizeof(rcvbuffer));
			if(bytes_read < 0)
				break;
		}
		//FIXME: This fails on some systems
		assert(strncmp(sendbuffer, rcvbuffer, sizeof(sendbuffer))==0);
		assert(sender.getAddress() == Address(127,0,0,1, 0).getAddress());
	}
};

struct TestConnection
{
	void TestHelpers()
	{
		/*
			Test helper functions
		*/

		// Some constants for testing
		u32 proto_id = 0x12345678;
		u16 peer_id = 123;
		u8 channel = 2;
		SharedBuffer<u8> data1(1);
		data1[0] = 100;
		Address a(127,0,0,1, 10);
		u16 seqnum = 34352;

		con::BufferedPacket p1 = con::makePacket(a, data1,
				proto_id, peer_id, channel);
		/*
			We should now have a packet with this data:
			Header:
				[0] u32 protocol_id
				[4] u16 sender_peer_id
				[6] u8 channel
			Data:
				[7] u8 data1[0]
		*/
		assert(readU32(&p1.data[0]) == proto_id);
		assert(readU16(&p1.data[4]) == peer_id);
		assert(readU8(&p1.data[6]) == channel);
		assert(readU8(&p1.data[7]) == data1[0]);
		
		//dstream<<"initial data1[0]="<<((u32)data1[0]&0xff)<<std::endl;

		SharedBuffer<u8> p2 = con::makeReliablePacket(data1, seqnum);

		/*dstream<<"p2.getSize()="<<p2.getSize()<<", data1.getSize()="
				<<data1.getSize()<<std::endl;
		dstream<<"readU8(&p2[3])="<<readU8(&p2[3])
				<<" p2[3]="<<((u32)p2[3]&0xff)<<std::endl;
		dstream<<"data1[0]="<<((u32)data1[0]&0xff)<<std::endl;*/

		assert(p2.getSize() == 3 + data1.getSize());
		assert(readU8(&p2[0]) == TYPE_RELIABLE);
		assert(readU16(&p2[1]) == seqnum);
		assert(readU8(&p2[3]) == data1[0]);
	}

	struct Handler : public con::PeerHandler
	{
		Handler(const char *a_name)
		{
			count = 0;
			last_id = 0;
			name = a_name;
		}
		void peerAdded(con::Peer *peer)
		{
			dstream<<"Handler("<<name<<")::peerAdded(): "
					"id="<<peer->id<<std::endl;
			last_id = peer->id;
			count++;
		}
		void deletingPeer(con::Peer *peer, bool timeout)
		{
			dstream<<"Handler("<<name<<")::deletingPeer(): "
					"id="<<peer->id
					<<", timeout="<<timeout<<std::endl;
			last_id = peer->id;
			count--;
		}

		s32 count;
		u16 last_id;
		const char *name;
	};

	void Run()
	{
		DSTACK("TestConnection::Run");

		TestHelpers();

		/*
			Test some real connections
		*/
		u32 proto_id = 0xad26846a;

		Handler hand_server("server");
		Handler hand_client("client");
		
		dstream<<"** Creating server Connection"<<std::endl;
		con::Connection server(proto_id, 512, 5.0, &hand_server);
		server.Serve(30001);
		
		dstream<<"** Creating client Connection"<<std::endl;
		con::Connection client(proto_id, 512, 5.0, &hand_client);

		assert(hand_server.count == 0);
		assert(hand_client.count == 0);
		
		sleep_ms(50);
		
		Address server_address(127,0,0,1, 30001);
		dstream<<"** running client.Connect()"<<std::endl;
		client.Connect(server_address);

		sleep_ms(50);
		
		// Client should have added server now
		assert(hand_client.count == 1);
		assert(hand_client.last_id == 1);
		// But server should not have added client
		assert(hand_server.count == 0);

		try
		{
			u16 peer_id;
			u8 data[100];
			dstream<<"** running server.Receive()"<<std::endl;
			u32 size = server.Receive(peer_id, data, 100);
			dstream<<"** Server received: peer_id="<<peer_id
					<<", size="<<size
					<<std::endl;
		}
		catch(con::NoIncomingDataException &e)
		{
			// No actual data received, but the client has
			// probably been connected
		}
		
		// Client should be the same
		assert(hand_client.count == 1);
		assert(hand_client.last_id == 1);
		// Server should have the client
		assert(hand_server.count == 1);
		assert(hand_server.last_id == 2);
		
		//sleep_ms(50);

		while(client.Connected() == false)
		{
			try
			{
				u16 peer_id;
				u8 data[100];
				dstream<<"** running client.Receive()"<<std::endl;
				u32 size = client.Receive(peer_id, data, 100);
				dstream<<"** Client received: peer_id="<<peer_id
						<<", size="<<size
						<<std::endl;
			}
			catch(con::NoIncomingDataException &e)
			{
			}
			sleep_ms(50);
		}

		sleep_ms(50);
		
		try
		{
			u16 peer_id;
			u8 data[100];
			dstream<<"** running server.Receive()"<<std::endl;
			u32 size = server.Receive(peer_id, data, 100);
			dstream<<"** Server received: peer_id="<<peer_id
					<<", size="<<size
					<<std::endl;
		}
		catch(con::NoIncomingDataException &e)
		{
		}

		{
			/*u8 data[] = "Hello World!";
			u32 datasize = sizeof(data);*/
			SharedBuffer<u8> data = SharedBufferFromString("Hello World!");

			dstream<<"** running client.Send()"<<std::endl;
			client.Send(PEER_ID_SERVER, 0, data, true);

			sleep_ms(50);

			u16 peer_id;
			u8 recvdata[100];
			dstream<<"** running server.Receive()"<<std::endl;
			u32 size = server.Receive(peer_id, recvdata, 100);
			dstream<<"** Server received: peer_id="<<peer_id
					<<", size="<<size
					<<", data="<<*data
					<<std::endl;
			assert(memcmp(*data, recvdata, data.getSize()) == 0);
		}
		
		u16 peer_id_client = 2;

		{
			/*
				Send consequent packets in different order
			*/
			//u8 data1[] = "hello1";
			//u8 data2[] = "hello2";
			SharedBuffer<u8> data1 = SharedBufferFromString("hello1");
			SharedBuffer<u8> data2 = SharedBufferFromString("Hello2");

			Address client_address =
					server.GetPeer(peer_id_client)->address;
			
			dstream<<"*** Sending packets in wrong order (2,1,2)"
					<<std::endl;
			
			u8 chn = 0;
			con::Channel *ch = &server.GetPeer(peer_id_client)->channels[chn];
			u16 sn = ch->next_outgoing_seqnum;
			ch->next_outgoing_seqnum = sn+1;
			server.Send(peer_id_client, chn, data2, true);
			ch->next_outgoing_seqnum = sn;
			server.Send(peer_id_client, chn, data1, true);
			ch->next_outgoing_seqnum = sn+1;
			server.Send(peer_id_client, chn, data2, true);

			sleep_ms(50);

			dstream<<"*** Receiving the packets"<<std::endl;

			u16 peer_id;
			u8 recvdata[20];
			u32 size;

			dstream<<"** running client.Receive()"<<std::endl;
			peer_id = 132;
			size = client.Receive(peer_id, recvdata, 20);
			dstream<<"** Client received: peer_id="<<peer_id
					<<", size="<<size
					<<", data="<<recvdata
					<<std::endl;
			assert(size == data1.getSize());
			assert(memcmp(*data1, recvdata, data1.getSize()) == 0);
			assert(peer_id == PEER_ID_SERVER);
			
			dstream<<"** running client.Receive()"<<std::endl;
			peer_id = 132;
			size = client.Receive(peer_id, recvdata, 20);
			dstream<<"** Client received: peer_id="<<peer_id
					<<", size="<<size
					<<", data="<<recvdata
					<<std::endl;
			assert(size == data2.getSize());
			assert(memcmp(*data2, recvdata, data2.getSize()) == 0);
			assert(peer_id == PEER_ID_SERVER);
			
			bool got_exception = false;
			try
			{
				dstream<<"** running client.Receive()"<<std::endl;
				peer_id = 132;
				size = client.Receive(peer_id, recvdata, 20);
				dstream<<"** Client received: peer_id="<<peer_id
						<<", size="<<size
						<<", data="<<recvdata
						<<std::endl;
			}
			catch(con::NoIncomingDataException &e)
			{
				dstream<<"** No incoming data for client"<<std::endl;
				got_exception = true;
			}
			assert(got_exception);
		}
		{
			const int datasize = 30000;
			SharedBuffer<u8> data1(datasize);
			for(u16 i=0; i<datasize; i++){
				data1[i] = i/4;
			}

			dstream<<"Sending data (size="<<datasize<<"):";
			for(int i=0; i<datasize && i<20; i++){
				if(i%2==0) DEBUGPRINT(" ");
				DEBUGPRINT("%.2X", ((int)((const char*)*data1)[i])&0xff);
			}
			if(datasize>20)
				dstream<<"...";
			dstream<<std::endl;
			
			server.Send(peer_id_client, 0, data1, true);

			sleep_ms(50);
			
			u8 recvdata[datasize + 1000];
			dstream<<"** running client.Receive()"<<std::endl;
			u16 peer_id = 132;
			u16 size = client.Receive(peer_id, recvdata, datasize + 1000);
			dstream<<"** Client received: peer_id="<<peer_id
					<<", size="<<size
					<<std::endl;

			dstream<<"Received data (size="<<size<<"):";
			for(int i=0; i<size && i<20; i++){
				if(i%2==0) DEBUGPRINT(" ");
				DEBUGPRINT("%.2X", ((int)((const char*)recvdata)[i])&0xff);
			}
			if(size>20)
				dstream<<"...";
			dstream<<std::endl;

			assert(memcmp(*data1, recvdata, data1.getSize()) == 0);
			assert(peer_id == PEER_ID_SERVER);
		}
		
		// Check peer handlers
		assert(hand_client.count == 1);
		assert(hand_client.last_id == 1);
		assert(hand_server.count == 1);
		assert(hand_server.last_id == 2);
		
		//assert(0);
	}
};

#define TEST(X)\
{\
	X x;\
	dstream<<"Running " #X <<std::endl;\
	x.Run();\
}

void run_tests()
{
	DSTACK(__FUNCTION_NAME);
	dstream<<"run_tests() started"<<std::endl;
	TEST(TestUtilities);
	TEST(TestCompress);
	TEST(TestMapNode);
	TEST(TestVoxelManipulator);
	TEST(TestMapBlock);
	TEST(TestMapSector);
	if(INTERNET_SIMULATOR == false){
		TEST(TestSocket);
		dout_con<<"=== BEGIN RUNNING UNIT TESTS FOR CONNECTION ==="<<std::endl;
		TEST(TestConnection);
		dout_con<<"=== END RUNNING UNIT TESTS FOR CONNECTION ==="<<std::endl;
	}
	dstream<<"run_tests() passed"<<std::endl;
}

