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
#include "content_mapnode.h"
#include "nodedef.h"
#include "mapsector.h"
#include "settings.h"
#include "log.h"
#include "utility_string.h"
#include "voxelalgorithms.h"
#include "inventory.h"

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

/*
	A few item and node definitions for those tests that need them
*/

#define CONTENT_STONE 0
#define CONTENT_GRASS 0x800
#define CONTENT_TORCH 100

void define_some_nodes(IWritableItemDefManager *idef, IWritableNodeDefManager *ndef)
{
	content_t i;
	ItemDefinition itemdef;
	ContentFeatures f;

	/*
		Stone
	*/
	i = CONTENT_STONE;
	itemdef = ItemDefinition();
	itemdef.type = ITEM_NODE;
	itemdef.name = "default:stone";
	itemdef.description = "Stone";
	itemdef.groups["cracky"] = 3;
	itemdef.inventory_image = "[inventorycube"
		"{default_stone.png"
		"{default_stone.png"
		"{default_stone.png";
	f = ContentFeatures();
	f.name = itemdef.name;
	for(int i = 0; i < 6; i++)
		f.tname_tiles[i] = "default_stone.png";
	f.is_ground_content = true;
	idef->registerItem(itemdef);
	ndef->set(i, f);

	/*
		Grass
	*/
	i = CONTENT_GRASS;
	itemdef = ItemDefinition();
	itemdef.type = ITEM_NODE;
	itemdef.name = "default:dirt_with_grass";
	itemdef.description = "Dirt with grass";
	itemdef.groups["crumbly"] = 3;
	itemdef.inventory_image = "[inventorycube"
		"{default_grass.png"
		"{default_dirt.png&default_grass_side.png"
		"{default_dirt.png&default_grass_side.png";
	f = ContentFeatures();
	f.name = itemdef.name;
	f.tname_tiles[0] = "default_grass.png";
	f.tname_tiles[1] = "default_dirt.png";
	for(int i = 2; i < 6; i++)
		f.tname_tiles[i] = "default_dirt.png^default_grass_side.png";
	f.is_ground_content = true;
	idef->registerItem(itemdef);
	ndef->set(i, f);

	/*
		Torch (minimal definition for lighting tests)
	*/
	i = CONTENT_TORCH;
	itemdef = ItemDefinition();
	itemdef.type = ITEM_NODE;
	itemdef.name = "default:torch";
	f = ContentFeatures();
	f.name = itemdef.name;
	f.param_type = CPT_LIGHT;
	f.light_propagates = true;
	f.sunlight_propagates = true;
	f.light_source = LIGHT_MAX-1;
	idef->registerItem(itemdef);
	ndef->set(i, f);
}

struct TestUtilities
{
	void Run()
	{
		/*infostream<<"wrapDegrees(100.0) = "<<wrapDegrees(100.0)<<std::endl;
		infostream<<"wrapDegrees(720.5) = "<<wrapDegrees(720.5)<<std::endl;
		infostream<<"wrapDegrees(-0.5) = "<<wrapDegrees(-0.5)<<std::endl;*/
		assert(fabs(wrapDegrees(100.0) - 100.0) < 0.001);
		assert(fabs(wrapDegrees(720.5) - 0.5) < 0.001);
		assert(fabs(wrapDegrees(-0.5) - (-0.5)) < 0.001);
		assert(fabs(wrapDegrees(-365.5) - (-5.5)) < 0.001);
		assert(lowercase("Foo bAR") == "foo bar");
		assert(is_yes("YeS") == true);
		assert(is_yes("") == false);
		assert(is_yes("FAlse") == false);
		const char *ends[] = {"abc", "c", "bc", NULL};
		assert(removeStringEnd("abc", ends) == "");
		assert(removeStringEnd("bc", ends) == "b");
		assert(removeStringEnd("12c", ends) == "12");
		assert(removeStringEnd("foo", ends) == "");
	}
};

struct TestSettings
{
	void Run()
	{
		Settings s;
		// Test reading of settings
		s.parseConfigLine("leet = 1337");
		s.parseConfigLine("leetleet = 13371337");
		s.parseConfigLine("leetleet_neg = -13371337");
		s.parseConfigLine("floaty_thing = 1.1");
		s.parseConfigLine("stringy_thing = asd /( ¤%&(/\" BLÖÄRP");
		s.parseConfigLine("coord = (1, 2, 4.5)");
		assert(s.getS32("leet") == 1337);
		assert(s.getS16("leetleet") == 32767);
		assert(s.getS16("leetleet_neg") == -32768);
		// Not sure if 1.1 is an exact value as a float, but doesn't matter
		assert(fabs(s.getFloat("floaty_thing") - 1.1) < 0.001);
		assert(s.get("stringy_thing") == "asd /( ¤%&(/\" BLÖÄRP");
		assert(fabs(s.getV3F("coord").X - 1.0) < 0.001);
		assert(fabs(s.getV3F("coord").Y - 2.0) < 0.001);
		assert(fabs(s.getV3F("coord").Z - 4.5) < 0.001);
		// Test the setting of settings too
		s.setFloat("floaty_thing_2", 1.2);
		s.setV3F("coord2", v3f(1, 2, 3.3));
		assert(s.get("floaty_thing_2").substr(0,3) == "1.2");
		assert(fabs(s.getFloat("floaty_thing_2") - 1.2) < 0.001);
		assert(fabs(s.getV3F("coord2").X - 1.0) < 0.001);
		assert(fabs(s.getV3F("coord2").Y - 2.0) < 0.001);
		assert(fabs(s.getV3F("coord2").Z - 3.3) < 0.001);
	}
};

struct TestSerialization
{
	// To be used like this:
	//   mkstr("Some\0string\0with\0embedded\0nuls")
	// since std::string("...") doesn't work as expected in that case.
	template<size_t N> std::string mkstr(const char (&s)[N])
	{
		return std::string(s, N - 1);
	}

	void Run()
	{
		// Tests some serialization primitives

		assert(serializeString("") == mkstr("\0\0"));
		assert(serializeWideString(L"") == mkstr("\0\0"));
		assert(serializeLongString("") == mkstr("\0\0\0\0"));
		assert(serializeJsonString("") == "\"\"");
		
		std::string teststring = "Hello world!";
		assert(serializeString(teststring) ==
			mkstr("\0\14Hello world!"));
		assert(serializeWideString(narrow_to_wide(teststring)) ==
			mkstr("\0\14\0H\0e\0l\0l\0o\0 \0w\0o\0r\0l\0d\0!"));
		assert(serializeLongString(teststring) ==
			mkstr("\0\0\0\14Hello world!"));
		assert(serializeJsonString(teststring) ==
			"\"Hello world!\"");

		std::string teststring2;
		std::wstring teststring2_w;
		std::string teststring2_w_encoded;
		{
			std::ostringstream tmp_os;
			std::wostringstream tmp_os_w;
			std::ostringstream tmp_os_w_encoded;
			for(int i = 0; i < 256; i++)
			{
				tmp_os<<(char)i;
				tmp_os_w<<(wchar_t)i;
				tmp_os_w_encoded<<(char)0<<(char)i;
			}
			teststring2 = tmp_os.str();
			teststring2_w = tmp_os_w.str();
			teststring2_w_encoded = tmp_os_w_encoded.str();
		}
		assert(serializeString(teststring2) ==
			mkstr("\1\0") + teststring2);
		assert(serializeWideString(teststring2_w) ==
			mkstr("\1\0") + teststring2_w_encoded);
		assert(serializeLongString(teststring2) ==
			mkstr("\0\0\1\0") + teststring2);
		// MSVC fails when directly using "\\\\"
		std::string backslash = "\\";
		assert(serializeJsonString(teststring2) ==
			mkstr("\"") +
			"\\u0000\\u0001\\u0002\\u0003\\u0004\\u0005\\u0006\\u0007" +
			"\\b\\t\\n\\u000b\\f\\r\\u000e\\u000f" +
			"\\u0010\\u0011\\u0012\\u0013\\u0014\\u0015\\u0016\\u0017" +
			"\\u0018\\u0019\\u001a\\u001b\\u001c\\u001d\\u001e\\u001f" +
			" !\\\"" + teststring2.substr(0x23, 0x2f-0x23) +
			"\\/" + teststring2.substr(0x30, 0x5c-0x30) +
			backslash + backslash + teststring2.substr(0x5d, 0x7f-0x5d) + "\\u007f" +
			"\\u0080\\u0081\\u0082\\u0083\\u0084\\u0085\\u0086\\u0087" +
			"\\u0088\\u0089\\u008a\\u008b\\u008c\\u008d\\u008e\\u008f" +
			"\\u0090\\u0091\\u0092\\u0093\\u0094\\u0095\\u0096\\u0097" +
			"\\u0098\\u0099\\u009a\\u009b\\u009c\\u009d\\u009e\\u009f" +
			"\\u00a0\\u00a1\\u00a2\\u00a3\\u00a4\\u00a5\\u00a6\\u00a7" +
			"\\u00a8\\u00a9\\u00aa\\u00ab\\u00ac\\u00ad\\u00ae\\u00af" +
			"\\u00b0\\u00b1\\u00b2\\u00b3\\u00b4\\u00b5\\u00b6\\u00b7" +
			"\\u00b8\\u00b9\\u00ba\\u00bb\\u00bc\\u00bd\\u00be\\u00bf" +
			"\\u00c0\\u00c1\\u00c2\\u00c3\\u00c4\\u00c5\\u00c6\\u00c7" +
			"\\u00c8\\u00c9\\u00ca\\u00cb\\u00cc\\u00cd\\u00ce\\u00cf" +
			"\\u00d0\\u00d1\\u00d2\\u00d3\\u00d4\\u00d5\\u00d6\\u00d7" +
			"\\u00d8\\u00d9\\u00da\\u00db\\u00dc\\u00dd\\u00de\\u00df" +
			"\\u00e0\\u00e1\\u00e2\\u00e3\\u00e4\\u00e5\\u00e6\\u00e7" +
			"\\u00e8\\u00e9\\u00ea\\u00eb\\u00ec\\u00ed\\u00ee\\u00ef" +
			"\\u00f0\\u00f1\\u00f2\\u00f3\\u00f4\\u00f5\\u00f6\\u00f7" +
			"\\u00f8\\u00f9\\u00fa\\u00fb\\u00fc\\u00fd\\u00fe\\u00ff" +
			"\"");

		{
			std::istringstream is(serializeString(teststring2), std::ios::binary);
			assert(deSerializeString(is) == teststring2);
			assert(!is.eof());
			is.get();
			assert(is.eof());
		}
		{
			std::istringstream is(serializeWideString(teststring2_w), std::ios::binary);
			assert(deSerializeWideString(is) == teststring2_w);
			assert(!is.eof());
			is.get();
			assert(is.eof());
		}
		{
			std::istringstream is(serializeLongString(teststring2), std::ios::binary);
			assert(deSerializeLongString(is) == teststring2);
			assert(!is.eof());
			is.get();
			assert(is.eof());
		}
		{
			std::istringstream is(serializeJsonString(teststring2), std::ios::binary);
			//dstream<<serializeJsonString(deSerializeJsonString(is));
			assert(deSerializeJsonString(is) == teststring2);
			assert(!is.eof());
			is.get();
			assert(is.eof());
		}
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
		
		infostream<<"str_out.size()="<<str_out.size()<<std::endl;
		infostream<<"TestCompress: 1,5,5,1 -> ";
		for(u32 i=0; i<str_out.size(); i++)
		{
			infostream<<(u32)str_out[i]<<",";
		}
		infostream<<std::endl;

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

		infostream<<"decompress: ";
		for(u32 i=0; i<str_out2.size(); i++)
		{
			infostream<<(u32)str_out2[i]<<",";
		}
		infostream<<std::endl;

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
		
		infostream<<"str_out.size()="<<str_out.size()<<std::endl;
		infostream<<"TestCompress: 1,5,5,1 -> ";
		for(u32 i=0; i<str_out.size(); i++)
		{
			infostream<<(u32)str_out[i]<<",";
		}
		infostream<<std::endl;

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

		infostream<<"decompress: ";
		for(u32 i=0; i<str_out2.size(); i++)
		{
			infostream<<(u32)str_out2[i]<<",";
		}
		infostream<<std::endl;

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
	void Run(INodeDefManager *nodedef)
	{
		MapNode n;

		// Default values
		assert(n.getContent() == CONTENT_AIR);
		assert(n.getLight(LIGHTBANK_DAY, nodedef) == 0);
		assert(n.getLight(LIGHTBANK_NIGHT, nodedef) == 0);
		
		// Transparency
		n.setContent(CONTENT_AIR);
		assert(nodedef->get(n).light_propagates == true);
		n.setContent(LEGN(nodedef, "CONTENT_STONE"));
		assert(nodedef->get(n).light_propagates == false);
	}
};

struct TestVoxelManipulator
{
	void Run(INodeDefManager *nodedef)
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
		
		infostream<<"Result of diff:"<<std::endl;
		for(core::list<VoxelArea>::Iterator
				i = aa.begin(); i != aa.end(); i++)
		{
			i->print(infostream);
			infostream<<std::endl;
			
			s32 j = results.linear_search(*i);
			assert(j != -1);
			results.erase(j, 1);
		}


		/*
			VoxelManipulator
		*/
		
		VoxelManipulator v;

		v.print(infostream, nodedef);

		infostream<<"*** Setting (-1,0,-1)=2 ***"<<std::endl;
		
		v.setNodeNoRef(v3s16(-1,0,-1), MapNode(CONTENT_GRASS));

		v.print(infostream, nodedef);

 		assert(v.getNode(v3s16(-1,0,-1)).getContent() == CONTENT_GRASS);

		infostream<<"*** Reading from inexistent (0,0,-1) ***"<<std::endl;

		EXCEPTION_CHECK(InvalidPositionException, v.getNode(v3s16(0,0,-1)));

		v.print(infostream, nodedef);

		infostream<<"*** Adding area ***"<<std::endl;

		v.addArea(a);
		
		v.print(infostream, nodedef);

		assert(v.getNode(v3s16(-1,0,-1)).getContent() == CONTENT_GRASS);
		EXCEPTION_CHECK(InvalidPositionException, v.getNode(v3s16(0,1,1)));
	}
};

struct TestVoxelAlgorithms
{
	void Run(INodeDefManager *ndef)
	{
		/*
			voxalgo::propagateSunlight
		*/
		{
			VoxelManipulator v;
			for(u16 z=0; z<3; z++)
			for(u16 y=0; y<3; y++)
			for(u16 x=0; x<3; x++)
			{
				v3s16 p(x,y,z);
				v.setNodeNoRef(p, MapNode(CONTENT_AIR));
			}
			VoxelArea a(v3s16(0,0,0), v3s16(2,2,2));
			{
				core::map<v3s16, bool> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, true, light_sources, ndef);
				//v.print(dstream, ndef, VOXELPRINT_LIGHT_DAY);
				assert(res.bottom_sunlight_valid == true);
				assert(v.getNode(v3s16(1,1,1)).getLight(LIGHTBANK_DAY, ndef)
						== LIGHT_SUN);
			}
			v.setNodeNoRef(v3s16(0,0,0), MapNode(CONTENT_STONE));
			{
				core::map<v3s16, bool> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, true, light_sources, ndef);
				assert(res.bottom_sunlight_valid == true);
				assert(v.getNode(v3s16(1,1,1)).getLight(LIGHTBANK_DAY, ndef)
						== LIGHT_SUN);
			}
			{
				core::map<v3s16, bool> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, false, light_sources, ndef);
				assert(res.bottom_sunlight_valid == true);
				assert(v.getNode(v3s16(2,0,2)).getLight(LIGHTBANK_DAY, ndef)
						== 0);
			}
			v.setNodeNoRef(v3s16(1,3,2), MapNode(CONTENT_STONE));
			{
				core::map<v3s16, bool> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, true, light_sources, ndef);
				assert(res.bottom_sunlight_valid == true);
				assert(v.getNode(v3s16(1,1,2)).getLight(LIGHTBANK_DAY, ndef)
						== 0);
			}
			{
				core::map<v3s16, bool> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, false, light_sources, ndef);
				assert(res.bottom_sunlight_valid == true);
				assert(v.getNode(v3s16(1,0,2)).getLight(LIGHTBANK_DAY, ndef)
						== 0);
			}
			{
				MapNode n(CONTENT_AIR);
				n.setLight(LIGHTBANK_DAY, 10, ndef);
				v.setNodeNoRef(v3s16(1,-1,2), n);
			}
			{
				core::map<v3s16, bool> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, true, light_sources, ndef);
				assert(res.bottom_sunlight_valid == true);
			}
			{
				core::map<v3s16, bool> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, false, light_sources, ndef);
				assert(res.bottom_sunlight_valid == true);
			}
			{
				MapNode n(CONTENT_AIR);
				n.setLight(LIGHTBANK_DAY, LIGHT_SUN, ndef);
				v.setNodeNoRef(v3s16(1,-1,2), n);
			}
			{
				core::map<v3s16, bool> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, true, light_sources, ndef);
				assert(res.bottom_sunlight_valid == false);
			}
			{
				core::map<v3s16, bool> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, false, light_sources, ndef);
				assert(res.bottom_sunlight_valid == false);
			}
			v.setNodeNoRef(v3s16(1,3,2), MapNode(CONTENT_IGNORE));
			{
				core::map<v3s16, bool> light_sources;
				voxalgo::setLight(v, a, 0, ndef);
				voxalgo::SunlightPropagateResult res = voxalgo::propagateSunlight(
						v, a, true, light_sources, ndef);
				assert(res.bottom_sunlight_valid == true);
			}
		}
		/*
			voxalgo::clearLightAndCollectSources
		*/
		{
			VoxelManipulator v;
			for(u16 z=0; z<3; z++)
			for(u16 y=0; y<3; y++)
			for(u16 x=0; x<3; x++)
			{
				v3s16 p(x,y,z);
				v.setNode(p, MapNode(CONTENT_AIR));
			}
			VoxelArea a(v3s16(0,0,0), v3s16(2,2,2));
			v.setNodeNoRef(v3s16(0,0,0), MapNode(CONTENT_STONE));
			v.setNodeNoRef(v3s16(1,1,1), MapNode(CONTENT_TORCH));
			{
				MapNode n(CONTENT_AIR);
				n.setLight(LIGHTBANK_DAY, 1, ndef);
				v.setNode(v3s16(1,1,2), n);
			}
			{
				core::map<v3s16, bool> light_sources;
				core::map<v3s16, u8> unlight_from;
				voxalgo::clearLightAndCollectSources(v, a, LIGHTBANK_DAY,
						ndef, light_sources, unlight_from);
				//v.print(dstream, ndef, VOXELPRINT_LIGHT_DAY);
				assert(v.getNode(v3s16(0,1,1)).getLight(LIGHTBANK_DAY, ndef)
						== 0);
				assert(light_sources.find(v3s16(1,1,1)) != NULL);
				assert(light_sources.size() == 1);
				assert(unlight_from.find(v3s16(1,1,2)) != NULL);
				assert(unlight_from.size() == 1);
			}
		}
	}
};

struct TestInventory
{
	void Run(IItemDefManager *idef)
	{
		std::string serialized_inventory =
		"List 0 32\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Item default:cobble 61\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Item default:dirt 71\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Item default:dirt 99\n"
		"Item default:cobble 38\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"EndInventoryList\n"
		"EndInventory\n";
		
		std::string serialized_inventory_2 =
		"List main 32\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Item default:cobble 61\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Item default:dirt 71\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Item default:dirt 99\n"
		"Item default:cobble 38\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"Empty\n"
		"EndInventoryList\n"
		"EndInventory\n";
		
		Inventory inv(idef);
		std::istringstream is(serialized_inventory, std::ios::binary);
		inv.deSerialize(is);
		assert(inv.getList("0"));
		assert(!inv.getList("main"));
		inv.getList("0")->setName("main");
		assert(!inv.getList("0"));
		assert(inv.getList("main"));
		std::ostringstream inv_os(std::ios::binary);
		inv.serialize(inv_os);
		assert(inv_os.str() == serialized_inventory_2);
	}
};

/*
	NOTE: These tests became non-working then NodeContainer was removed.
	      These should be redone, utilizing some kind of a virtual
		  interface for Map (IMap would be fine).
*/
#if 0
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
		assert(b.getModified() == MOD_STATE_WRITE_NEEDED);
		b.resetModified();
		assert(b.getModified() == MOD_STATE_CLEAN);

		// All nodes should have been set to
		// .d=CONTENT_IGNORE and .getLight() = 0
		for(u16 z=0; z<MAP_BLOCKSIZE; z++)
		for(u16 y=0; y<MAP_BLOCKSIZE; y++)
		for(u16 x=0; x<MAP_BLOCKSIZE; x++)
		{
			//assert(b.getNode(v3s16(x,y,z)).getContent() == CONTENT_AIR);
			assert(b.getNode(v3s16(x,y,z)).getContent() == CONTENT_IGNORE);
			assert(b.getNode(v3s16(x,y,z)).getLight(LIGHTBANK_DAY) == 0);
			assert(b.getNode(v3s16(x,y,z)).getLight(LIGHTBANK_NIGHT) == 0);
		}
		
		{
			MapNode n(CONTENT_AIR);
			for(u16 z=0; z<MAP_BLOCKSIZE; z++)
			for(u16 y=0; y<MAP_BLOCKSIZE; y++)
			for(u16 x=0; x<MAP_BLOCKSIZE; x++)
			{
				b.setNode(v3s16(x,y,z), n);
			}
		}
			
		/*
			Parent fetch functions
		*/
		parent.position_valid = false;
		parent.node.setContent(5);

		MapNode n;
		
		// Positions in the block should still be valid
		assert(b.isValidPositionParent(v3s16(0,0,0)) == true);
		assert(b.isValidPositionParent(v3s16(MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1,MAP_BLOCKSIZE-1)) == true);
		n = b.getNodeParent(v3s16(0,MAP_BLOCKSIZE-1,0));
		assert(n.getContent() == CONTENT_AIR);

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
		assert(n.getContent() == 5);

		/*
			Set a node
		*/
		v3s16 p(1,2,0);
		n.setContent(4);
		b.setNode(p, n);
		assert(b.getNode(p).getContent() == 4);
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
			parent.node.setContent(CONTENT_AIR);
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
						n.setContent(CONTENT_AIR);
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
		
		assert(sector.getBlockNoCreateNoEx(0) == 0);
		assert(sector.getBlockNoCreateNoEx(1) == 0);

		MapBlock * bref = sector.createBlankBlock(-2);
		
		assert(sector.getBlockNoCreateNoEx(0) == 0);
		assert(sector.getBlockNoCreateNoEx(-2) == bref);
		
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
#endif

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
		
		//infostream<<"initial data1[0]="<<((u32)data1[0]&0xff)<<std::endl;

		SharedBuffer<u8> p2 = con::makeReliablePacket(data1, seqnum);

		/*infostream<<"p2.getSize()="<<p2.getSize()<<", data1.getSize()="
				<<data1.getSize()<<std::endl;
		infostream<<"readU8(&p2[3])="<<readU8(&p2[3])
				<<" p2[3]="<<((u32)p2[3]&0xff)<<std::endl;
		infostream<<"data1[0]="<<((u32)data1[0]&0xff)<<std::endl;*/

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
			infostream<<"Handler("<<name<<")::peerAdded(): "
					"id="<<peer->id<<std::endl;
			last_id = peer->id;
			count++;
		}
		void deletingPeer(con::Peer *peer, bool timeout)
		{
			infostream<<"Handler("<<name<<")::deletingPeer(): "
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

			NOTE: This mostly tests the legacy interface.
		*/

		u32 proto_id = 0xad26846a;

		Handler hand_server("server");
		Handler hand_client("client");
		
		infostream<<"** Creating server Connection"<<std::endl;
		con::Connection server(proto_id, 512, 5.0, &hand_server);
		server.Serve(30001);
		
		infostream<<"** Creating client Connection"<<std::endl;
		con::Connection client(proto_id, 512, 5.0, &hand_client);

		assert(hand_server.count == 0);
		assert(hand_client.count == 0);
		
		sleep_ms(50);
		
		Address server_address(127,0,0,1, 30001);
		infostream<<"** running client.Connect()"<<std::endl;
		client.Connect(server_address);

		sleep_ms(50);
		
		// Client should not have added client yet
		assert(hand_client.count == 0);
		
		try
		{
			u16 peer_id;
			SharedBuffer<u8> data;
			infostream<<"** running client.Receive()"<<std::endl;
			u32 size = client.Receive(peer_id, data);
			infostream<<"** Client received: peer_id="<<peer_id
					<<", size="<<size
					<<std::endl;
		}
		catch(con::NoIncomingDataException &e)
		{
		}

		// Client should have added server now
		assert(hand_client.count == 1);
		assert(hand_client.last_id == 1);
		// Server should not have added client yet
		assert(hand_server.count == 0);
		
		sleep_ms(50);

		try
		{
			u16 peer_id;
			SharedBuffer<u8> data;
			infostream<<"** running server.Receive()"<<std::endl;
			u32 size = server.Receive(peer_id, data);
			infostream<<"** Server received: peer_id="<<peer_id
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
				SharedBuffer<u8> data;
				infostream<<"** running client.Receive()"<<std::endl;
				u32 size = client.Receive(peer_id, data);
				infostream<<"** Client received: peer_id="<<peer_id
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
			SharedBuffer<u8> data;
			infostream<<"** running server.Receive()"<<std::endl;
			u32 size = server.Receive(peer_id, data);
			infostream<<"** Server received: peer_id="<<peer_id
					<<", size="<<size
					<<std::endl;
		}
		catch(con::NoIncomingDataException &e)
		{
		}
#if 1
		/*
			Simple send-receive test
		*/
		{
			/*u8 data[] = "Hello World!";
			u32 datasize = sizeof(data);*/
			SharedBuffer<u8> data = SharedBufferFromString("Hello World!");

			infostream<<"** running client.Send()"<<std::endl;
			client.Send(PEER_ID_SERVER, 0, data, true);

			sleep_ms(50);

			u16 peer_id;
			SharedBuffer<u8> recvdata;
			infostream<<"** running server.Receive()"<<std::endl;
			u32 size = server.Receive(peer_id, recvdata);
			infostream<<"** Server received: peer_id="<<peer_id
					<<", size="<<size
					<<", data="<<*data
					<<std::endl;
			assert(memcmp(*data, *recvdata, data.getSize()) == 0);
		}
#endif
		u16 peer_id_client = 2;
#if 0
		/*
			Send consequent packets in different order
			Not compatible with new Connection, thus commented out.
		*/
		{
			//u8 data1[] = "hello1";
			//u8 data2[] = "hello2";
			SharedBuffer<u8> data1 = SharedBufferFromString("hello1");
			SharedBuffer<u8> data2 = SharedBufferFromString("Hello2");

			Address client_address =
					server.GetPeerAddress(peer_id_client);
			
			infostream<<"*** Sending packets in wrong order (2,1,2)"
					<<std::endl;
			
			u8 chn = 0;
			con::Channel *ch = &server.getPeer(peer_id_client)->channels[chn];
			u16 sn = ch->next_outgoing_seqnum;
			ch->next_outgoing_seqnum = sn+1;
			server.Send(peer_id_client, chn, data2, true);
			ch->next_outgoing_seqnum = sn;
			server.Send(peer_id_client, chn, data1, true);
			ch->next_outgoing_seqnum = sn+1;
			server.Send(peer_id_client, chn, data2, true);

			sleep_ms(50);

			infostream<<"*** Receiving the packets"<<std::endl;

			u16 peer_id;
			SharedBuffer<u8> recvdata;
			u32 size;

			infostream<<"** running client.Receive()"<<std::endl;
			peer_id = 132;
			size = client.Receive(peer_id, recvdata);
			infostream<<"** Client received: peer_id="<<peer_id
					<<", size="<<size
					<<", data="<<*recvdata
					<<std::endl;
			assert(size == data1.getSize());
			assert(memcmp(*data1, *recvdata, data1.getSize()) == 0);
			assert(peer_id == PEER_ID_SERVER);
			
			infostream<<"** running client.Receive()"<<std::endl;
			peer_id = 132;
			size = client.Receive(peer_id, recvdata);
			infostream<<"** Client received: peer_id="<<peer_id
					<<", size="<<size
					<<", data="<<*recvdata
					<<std::endl;
			assert(size == data2.getSize());
			assert(memcmp(*data2, *recvdata, data2.getSize()) == 0);
			assert(peer_id == PEER_ID_SERVER);
			
			bool got_exception = false;
			try
			{
				infostream<<"** running client.Receive()"<<std::endl;
				peer_id = 132;
				size = client.Receive(peer_id, recvdata);
				infostream<<"** Client received: peer_id="<<peer_id
						<<", size="<<size
						<<", data="<<*recvdata
						<<std::endl;
			}
			catch(con::NoIncomingDataException &e)
			{
				infostream<<"** No incoming data for client"<<std::endl;
				got_exception = true;
			}
			assert(got_exception);
		}
#endif
#if 0
		/*
			Send large amounts of packets (infinite test)
			Commented out because of infinity.
		*/
		{
			infostream<<"Sending large amounts of packets (infinite test)"<<std::endl;
			int sendcount = 0;
			for(;;){
				int datasize = myrand_range(0,5)==0?myrand_range(100,10000):myrand_range(0,100);
				infostream<<"datasize="<<datasize<<std::endl;
				SharedBuffer<u8> data1(datasize);
				for(u16 i=0; i<datasize; i++)
					data1[i] = i/4;
				
				int sendtimes = myrand_range(1,10);
				for(int i=0; i<sendtimes; i++){
					server.Send(peer_id_client, 0, data1, true);
					sendcount++;
				}
				infostream<<"sendcount="<<sendcount<<std::endl;
				
				//int receivetimes = myrand_range(1,20);
				int receivetimes = 20;
				for(int i=0; i<receivetimes; i++){
					SharedBuffer<u8> recvdata;
					u16 peer_id = 132;
					u16 size = 0;
					bool received = false;
					try{
						size = client.Receive(peer_id, recvdata);
						received = true;
					}catch(con::NoIncomingDataException &e){
					}
				}
			}
		}
#endif
		/*
			Send a large packet
		*/
		{
			const int datasize = 30000;
			SharedBuffer<u8> data1(datasize);
			for(u16 i=0; i<datasize; i++){
				data1[i] = i/4;
			}

			infostream<<"Sending data (size="<<datasize<<"):";
			for(int i=0; i<datasize && i<20; i++){
				if(i%2==0) infostream<<" ";
				char buf[10];
				snprintf(buf, 10, "%.2X", ((int)((const char*)*data1)[i])&0xff);
				infostream<<buf;
			}
			if(datasize>20)
				infostream<<"...";
			infostream<<std::endl;
			
			server.Send(peer_id_client, 0, data1, true);

			//sleep_ms(3000);
			
			SharedBuffer<u8> recvdata;
			infostream<<"** running client.Receive()"<<std::endl;
			u16 peer_id = 132;
			u16 size = 0;
			bool received = false;
			u32 timems0 = porting::getTimeMs();
			for(;;){
				if(porting::getTimeMs() - timems0 > 5000 || received)
					break;
				try{
					size = client.Receive(peer_id, recvdata);
					received = true;
				}catch(con::NoIncomingDataException &e){
				}
				sleep_ms(10);
			}
			assert(received);
			infostream<<"** Client received: peer_id="<<peer_id
					<<", size="<<size
					<<std::endl;

			infostream<<"Received data (size="<<size<<"): ";
			for(int i=0; i<size && i<20; i++){
				if(i%2==0) infostream<<" ";
				char buf[10];
				snprintf(buf, 10, "%.2X", ((int)(recvdata[i]))&0xff);
				infostream<<buf;
			}
			if(size>20)
				infostream<<"...";
			infostream<<std::endl;

			assert(memcmp(*data1, *recvdata, data1.getSize()) == 0);
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
	infostream<<"Running " #X <<std::endl;\
	x.Run();\
}

#define TESTPARAMS(X, ...)\
{\
	X x;\
	infostream<<"Running " #X <<std::endl;\
	x.Run(__VA_ARGS__);\
}

void run_tests()
{
	DSTACK(__FUNCTION_NAME);
	
	// Create item and node definitions
	IWritableItemDefManager *idef = createItemDefManager();
	IWritableNodeDefManager *ndef = createNodeDefManager();
	define_some_nodes(idef, ndef);

	infostream<<"run_tests() started"<<std::endl;
	TEST(TestUtilities);
	TEST(TestSettings);
	TEST(TestCompress);
	TEST(TestSerialization);
	TESTPARAMS(TestMapNode, ndef);
	TESTPARAMS(TestVoxelManipulator, ndef);
	TESTPARAMS(TestVoxelAlgorithms, ndef);
	TESTPARAMS(TestInventory, idef);
	//TEST(TestMapBlock);
	//TEST(TestMapSector);
	if(INTERNET_SIMULATOR == false){
		TEST(TestSocket);
		dout_con<<"=== BEGIN RUNNING UNIT TESTS FOR CONNECTION ==="<<std::endl;
		TEST(TestConnection);
		dout_con<<"=== END RUNNING UNIT TESTS FOR CONNECTION ==="<<std::endl;
	}
	infostream<<"run_tests() passed"<<std::endl;
}

