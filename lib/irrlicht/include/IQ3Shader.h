// Copyright (C) 2006-2012 Nikolaus Gebhardt / Thomas Alten
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_Q3_LEVEL_SHADER_H_INCLUDED__
#define __I_Q3_LEVEL_SHADER_H_INCLUDED__

#include "irrArray.h"
#include "fast_atof.h"
#include "IFileSystem.h"
#include "IVideoDriver.h"
#include "coreutil.h"

namespace irr
{
namespace scene
{
namespace quake3
{

	static core::stringc irrEmptyStringc("");

	//! Hold the different Mesh Types used for getMesh
	enum eQ3MeshIndex
	{
		E_Q3_MESH_GEOMETRY = 0,
		E_Q3_MESH_ITEMS,
		E_Q3_MESH_BILLBOARD,
		E_Q3_MESH_FOG,
		E_Q3_MESH_UNRESOLVED,
		E_Q3_MESH_SIZE
	};

	/*! used to customize Quake3 BSP Loader
	*/

	struct Q3LevelLoadParameter
	{
		Q3LevelLoadParameter ()
			:defaultLightMapMaterial ( video::EMT_LIGHTMAP_M4 ),
			defaultModulate ( video::EMFN_MODULATE_4X ),
			defaultFilter ( video::EMF_BILINEAR_FILTER ),
			patchTesselation ( 8 ),
			verbose ( 0 ),
			startTime ( 0 ), endTime ( 0 ),
			mergeShaderBuffer ( 1 ),
			cleanUnResolvedMeshes ( 1 ),
			loadAllShaders ( 0 ),
			loadSkyShader ( 0 ),
			alpharef ( 1 ),
			swapLump ( 0 ),
	#ifdef __BIG_ENDIAN__
			swapHeader ( 1 )
	#else
			swapHeader ( 0 )
	#endif
			{
				memcpy ( scriptDir, "scripts\x0", 8 );
			}

		video::E_MATERIAL_TYPE defaultLightMapMaterial;
		video::E_MODULATE_FUNC defaultModulate;
		video::E_MATERIAL_FLAG defaultFilter;
		s32 patchTesselation;
		s32 verbose;
		u32 startTime;
		u32 endTime;
		s32 mergeShaderBuffer;
		s32 cleanUnResolvedMeshes;
		s32 loadAllShaders;
		s32 loadSkyShader;
		s32 alpharef;
		s32 swapLump;
		s32 swapHeader;
		c8 scriptDir [ 64 ];
	};

	// some useful typedefs
	typedef core::array< core::stringc > tStringList;
	typedef core::array< video::ITexture* > tTexArray;

	// string helper.. TODO: move to generic files
	inline s16 isEqual ( const core::stringc &string, u32 &pos, const c8 *list[], u16 listSize )
	{
		const char * in = string.c_str () + pos;

		for ( u16 i = 0; i != listSize; ++i )
		{
			if (string.size() < pos)
				return -2;
			u32 len = (u32) strlen ( list[i] );
			if (string.size() < pos+len)
				continue;
			if ( in [len] != 0 && in [len] != ' ' )
				continue;
			if ( strncmp ( in, list[i], len ) )
				continue;

			pos += len + 1;
			return (s16) i;
		}
		return -2;
	}

	inline f32 getAsFloat ( const core::stringc &string, u32 &pos )
	{
		const char * in = string.c_str () + pos;

		f32 value = 0.f;
		pos += (u32) ( core::fast_atof_move ( in, value ) - in ) + 1;
		return value;
	}

	//! get a quake3 vector translated to irrlicht position (x,-z,y )
	inline core::vector3df getAsVector3df ( const core::stringc &string, u32 &pos )
	{
		core::vector3df v;

		v.X = getAsFloat ( string, pos );
		v.Z = getAsFloat ( string, pos );
		v.Y = getAsFloat ( string, pos );

		return v;
	}


	/*
		extract substrings
	*/
	inline void getAsStringList ( tStringList &list, s32 max, const core::stringc &string, u32 &startPos )
	{
		list.clear ();

		s32 finish = 0;
		s32 endPos;
		do
		{
			endPos = string.findNext ( ' ', startPos );
			if ( endPos == -1 )
			{
				finish = 1;
				endPos = string.size();
			}

			list.push_back ( string.subString ( startPos, endPos - startPos ) );
			startPos = endPos + 1;

			if ( list.size() >= (u32) max )
				finish = 1;

		} while ( !finish );

	}

	//! A blend function for a q3 shader.
	struct SBlendFunc
	{
		SBlendFunc ( video::E_MODULATE_FUNC mod )
			: type ( video::EMT_SOLID ), modulate ( mod ),
				param0( 0.f ),
			isTransparent ( 0 ) {}

		video::E_MATERIAL_TYPE type;
		video::E_MODULATE_FUNC modulate;

		f32 param0;
		u32 isTransparent;
	};

	// parses the content of Variable cull
	inline bool getCullingFunction ( const core::stringc &cull )
	{
		if ( cull.size() == 0 )
			return true;

		bool ret = true;
		static const c8 * funclist[] = { "none", "disable", "twosided" };

		u32 pos = 0;
		switch ( isEqual ( cull, pos, funclist, 3 ) )
		{
			case 0:
			case 1:
			case 2:
				ret = false;
				break;
		}
		return ret;
	}

	// parses the content of Variable depthfunc
	// return a z-test
	inline u8 getDepthFunction ( const core::stringc &string )
	{
		u8 ret = video::ECFN_LESSEQUAL;

		if ( string.size() == 0 )
			return ret;

		static const c8 * funclist[] = { "lequal","equal" };

		u32 pos = 0;
		switch ( isEqual ( string, pos, funclist, 2 ) )
		{
			case 0:
				ret = video::ECFN_LESSEQUAL;
				break;
			case 1:
				ret = video::ECFN_EQUAL;
				break;
		}
		return ret;
	}


	/*!
		parses the content of Variable blendfunc,alphafunc
		it also make a hint for rendering as transparent or solid node.

		we assume a typical quake scene would look like this..
		1) Big Static Mesh ( solid )
		2) static scene item ( may use transparency ) but rendered in the solid pass
		3) additional transparency item in the transparent pass

		it's not 100% accurate! it just empirical..
	*/
	inline static void getBlendFunc ( const core::stringc &string, SBlendFunc &blendfunc )
	{
		if ( string.size() == 0 )
			return;

		// maps to E_BLEND_FACTOR
		static const c8 * funclist[] =
		{
			"gl_zero",
			"gl_one",
			"gl_dst_color",
			"gl_one_minus_dst_color",
			"gl_src_color",
			"gl_one_minus_src_color",
			"gl_src_alpha",
			"gl_one_minus_src_alpha",
			"gl_dst_alpha",
			"gl_one_minus_dst_alpha",
			"gl_src_alpha_sat",

			"add",
			"filter",
			"blend",

			"ge128",
			"gt0",
		};


		u32 pos = 0;
		s32 srcFact = isEqual ( string, pos, funclist, 16 );

		if ( srcFact < 0 )
			return;

		u32 resolved = 0;
		s32 dstFact = isEqual ( string, pos, funclist, 16 );

		switch ( srcFact )
		{
			case video::EBF_ZERO:
				switch ( dstFact )
				{
					// gl_zero gl_src_color == gl_dst_color gl_zero
					case video::EBF_SRC_COLOR:
						blendfunc.type = video::EMT_ONETEXTURE_BLEND;
						blendfunc.param0 = video::pack_textureBlendFunc ( video::EBF_DST_COLOR, video::EBF_ZERO, blendfunc.modulate );
						blendfunc.isTransparent = 1;
						resolved = 1;
						break;
				} break;

			case video::EBF_ONE:
				switch ( dstFact )
				{
					// gl_one gl_zero
					case video::EBF_ZERO:
						blendfunc.type = video::EMT_SOLID;
						blendfunc.isTransparent = 0;
						resolved = 1;
						break;

					// gl_one gl_one
					case video::EBF_ONE:
						blendfunc.type = video::EMT_TRANSPARENT_ADD_COLOR;
						blendfunc.isTransparent = 1;
						resolved = 1;
						break;
				} break;

			case video::EBF_SRC_ALPHA:
				switch ( dstFact )
				{
					// gl_src_alpha gl_one_minus_src_alpha
					case video::EBF_ONE_MINUS_SRC_ALPHA:
						blendfunc.type = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
						blendfunc.param0 = 1.f/255.f;
						blendfunc.isTransparent = 1;
						resolved = 1;
						break;
				} break;

			case 11:
				// add
				blendfunc.type = video::EMT_TRANSPARENT_ADD_COLOR;
				blendfunc.isTransparent = 1;
				resolved = 1;
				break;
			case 12:
				// filter = gl_dst_color gl_zero or gl_zero gl_src_color
				blendfunc.type = video::EMT_ONETEXTURE_BLEND;
				blendfunc.param0 = video::pack_textureBlendFunc ( video::EBF_DST_COLOR, video::EBF_ZERO, blendfunc.modulate );
				blendfunc.isTransparent = 1;
				resolved = 1;
				break;
			case 13:
				// blend = gl_src_alpha gl_one_minus_src_alpha
				blendfunc.type = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
				blendfunc.param0 = 1.f/255.f;
				blendfunc.isTransparent = 1;
				resolved = 1;
				break;
			case 14:
				// alphafunc ge128
				blendfunc.type = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
				blendfunc.param0 = 0.5f;
				blendfunc.isTransparent = 1;
				resolved = 1;
				break;
			case 15:
				// alphafunc gt0
				blendfunc.type = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
				blendfunc.param0 = 1.f / 255.f;
				blendfunc.isTransparent = 1;
				resolved = 1;
				break;

		}

		// use the generic blender
		if ( 0 == resolved )
		{
			blendfunc.type = video::EMT_ONETEXTURE_BLEND;
			blendfunc.param0 = video::pack_textureBlendFunc (
					(video::E_BLEND_FACTOR) srcFact,
					(video::E_BLEND_FACTOR) dstFact,
					blendfunc.modulate);

			blendfunc.isTransparent = 1;
		}
	}

	// random noise [-1;1]
	struct Noiser
	{
		static f32 get ()
		{
			static u32 RandomSeed = 0x69666966;
			RandomSeed = (RandomSeed * 3631 + 1);

			f32 value = ( (f32) (RandomSeed & 0x7FFF ) * (1.0f / (f32)(0x7FFF >> 1) ) ) - 1.f;
			return value;
		}
	};

	enum eQ3ModifierFunction
	{
		TCMOD				= 0,
		DEFORMVERTEXES		= 1,
		RGBGEN				= 2,
		TCGEN				= 3,
		MAP					= 4,
		ALPHAGEN			= 5,

		FUNCTION2			= 0x10,
		SCROLL				= FUNCTION2 + 1,
		SCALE				= FUNCTION2 + 2,
		ROTATE				= FUNCTION2 + 3,
		STRETCH				= FUNCTION2 + 4,
		TURBULENCE			= FUNCTION2 + 5,
		WAVE				= FUNCTION2 + 6,

		IDENTITY			= FUNCTION2 + 7,
		VERTEX				= FUNCTION2 + 8,
		TEXTURE				= FUNCTION2 + 9,
		LIGHTMAP			= FUNCTION2 + 10,
		ENVIRONMENT			= FUNCTION2 + 11,
		DOLLAR_LIGHTMAP		= FUNCTION2 + 12,
		BULGE				= FUNCTION2 + 13,
		AUTOSPRITE			= FUNCTION2 + 14,
		AUTOSPRITE2			= FUNCTION2 + 15,
		TRANSFORM			= FUNCTION2 + 16,
		EXACTVERTEX			= FUNCTION2 + 17,
		CONSTANT			= FUNCTION2 + 18,
		LIGHTINGSPECULAR	= FUNCTION2 + 19,
		MOVE				= FUNCTION2 + 20,
		NORMAL				= FUNCTION2 + 21,
		IDENTITYLIGHTING	= FUNCTION2 + 22,

		WAVE_MODIFIER_FUNCTION	= 0x30,
		SINUS				= WAVE_MODIFIER_FUNCTION + 1,
		COSINUS				= WAVE_MODIFIER_FUNCTION + 2,
		SQUARE				= WAVE_MODIFIER_FUNCTION + 3,
		TRIANGLE			= WAVE_MODIFIER_FUNCTION + 4,
		SAWTOOTH			= WAVE_MODIFIER_FUNCTION + 5,
		SAWTOOTH_INVERSE	= WAVE_MODIFIER_FUNCTION + 6,
		NOISE				= WAVE_MODIFIER_FUNCTION + 7,


		UNKNOWN				= -2

	};

	struct SModifierFunction
	{
		SModifierFunction ()
			: masterfunc0 ( UNKNOWN ), masterfunc1( UNKNOWN ), func ( SINUS ),
			tcgen( TEXTURE ), rgbgen ( IDENTITY ), alphagen ( UNKNOWN ),
			base ( 0 ), amp ( 1 ), phase ( 0 ), frequency ( 1 ),
			wave ( 1 ),
			x ( 0 ), y ( 0 ), z( 0 ), count( 0 ) {}

		// "tcmod","deformvertexes","rgbgen", "tcgen"
		eQ3ModifierFunction masterfunc0;
		// depends
		eQ3ModifierFunction masterfunc1;
		// depends
		eQ3ModifierFunction func;

		eQ3ModifierFunction tcgen;
		eQ3ModifierFunction rgbgen;
		eQ3ModifierFunction alphagen;

		union
		{
			f32 base;
			f32 bulgewidth;
		};

		union
		{
			f32 amp;
			f32 bulgeheight;
		};

		f32 phase;

		union
		{
			f32 frequency;
			f32 bulgespeed;
		};

		union
		{
			f32 wave;
			f32 div;
		};

		f32 x;
		f32 y;
		f32 z;
		u32 count;

		f32 evaluate ( f32 dt ) const
		{
			// phase in 0 and 1..
			f32 x = core::fract( (dt + phase ) * frequency );
			f32 y = 0.f;

			switch ( func )
			{
				case SINUS:
					y = sinf ( x * core::PI * 2.f );
					break;
				case COSINUS:
					y = cosf ( x * core::PI * 2.f );
					break;
				case SQUARE:
					y = x < 0.5f ? 1.f : -1.f;
					break;
				case TRIANGLE:
					y = x < 0.5f ? ( 4.f * x ) - 1.f : ( -4.f * x ) + 3.f;
					break;
				case SAWTOOTH:
					y = x;
					break;
				case SAWTOOTH_INVERSE:
					y = 1.f - x;
					break;
				case NOISE:
					y = Noiser::get();
					break;
				default:
					break;
			}

			return base + ( y * amp );
		}


	};

	inline core::vector3df getMD3Normal ( u32 i, u32 j )
	{
		const f32 lng = i * 2.0f * core::PI / 255.0f;
		const f32 lat = j * 2.0f * core::PI / 255.0f;
		return core::vector3df(cosf ( lat ) * sinf ( lng ),
				sinf ( lat ) * sinf ( lng ),
				cosf ( lng ));
	}

	//
	inline void getModifierFunc ( SModifierFunction& fill, const core::stringc &string, u32 &pos )
	{
		if ( string.size() == 0 )
			return;

		static const c8 * funclist[] =
		{
			"sin","cos","square",
			"triangle", "sawtooth","inversesawtooth", "noise"
		};

		fill.func = (eQ3ModifierFunction) isEqual ( string,pos, funclist,7 );
		fill.func = fill.func == UNKNOWN ? SINUS : (eQ3ModifierFunction) ((u32) fill.func + WAVE_MODIFIER_FUNCTION + 1);

		fill.base = getAsFloat ( string, pos );
		fill.amp = getAsFloat ( string, pos );
		fill.phase = getAsFloat ( string, pos );
		fill.frequency = getAsFloat ( string, pos );
	}


	// name = "a b c .."
	struct SVariable
	{
		core::stringc name;
		core::stringc content;

		SVariable ( const c8 * n, const c8 *c = 0 ) : name ( n ), content (c) {}
		virtual ~SVariable () {}

		void clear ()
		{
			name = "";
			content = "";
		}

		s32 isValid () const
		{
			return name.size();
		}

		bool operator == ( const SVariable &other ) const
		{
			return 0 == strcmp ( name.c_str(), other.name.c_str () );
		}

		bool operator < ( const SVariable &other ) const
		{
			return 0 > strcmp ( name.c_str(), other.name.c_str () );
		}

	};


	// string database. "a" = "Hello", "b" = "1234.6"
	struct SVarGroup
	{
		SVarGroup () { Variable.setAllocStrategy ( core::ALLOC_STRATEGY_SAFE ); }
		virtual ~SVarGroup () {}

		u32 isDefined ( const c8 * name, const c8 * content = 0 ) const
		{
			for ( u32 i = 0; i != Variable.size (); ++i )
			{
				if ( 0 == strcmp ( Variable[i].name.c_str(), name ) &&
					(  0 == content || strstr ( Variable[i].content.c_str(), content ) )
					)
				{
					return i + 1;
				}
			}
			return 0;
		}

		// searches for Variable name and returns is content
		// if Variable is not found a reference to an Empty String is returned
		const core::stringc &get( const c8 * name ) const
		{
			SVariable search ( name );
			s32 index = Variable.linear_search ( search );
			if ( index < 0 )
				return irrEmptyStringc;

			return Variable [ index ].content;
		}

		// set the Variable name
		void set ( const c8 * name, const c8 * content = 0 )
		{
			u32 index = isDefined ( name, 0 );
			if ( 0 == index )
			{
				Variable.push_back ( SVariable ( name, content ) );
			}
			else
			{
				Variable [ index ].content = content;
			}
		}


		core::array < SVariable > Variable;
	};

	//! holding a group a variable
	struct SVarGroupList: public IReferenceCounted
	{
		SVarGroupList ()
		{
			VariableGroup.setAllocStrategy ( core::ALLOC_STRATEGY_SAFE );
		}
		virtual ~SVarGroupList () {}

		core::array < SVarGroup > VariableGroup;
	};


	//! A Parsed Shader Holding Variables ordered in Groups
	struct IShader
	{
		IShader ()
			: ID ( 0 ), VarGroup ( 0 )  {}
		virtual ~IShader () {}

		void operator = (const IShader &other )
		{
			ID = other.ID;
			VarGroup = other.VarGroup;
			name = other.name;
		}

		bool operator == (const IShader &other ) const
		{
			return 0 == strcmp ( name.c_str(), other.name.c_str () );
			//return name == other.name;
		}

		bool operator < (const IShader &other ) const
		{
			return strcmp ( name.c_str(), other.name.c_str () ) < 0;
			//return name < other.name;
		}

		u32 getGroupSize () const
		{
			if ( 0 == VarGroup )
				return 0;
			return VarGroup->VariableGroup.size ();
		}

		const SVarGroup * getGroup ( u32 stage ) const
		{
			if ( 0 == VarGroup || stage >= VarGroup->VariableGroup.size () )
				return 0;

			return &VarGroup->VariableGroup [ stage ];
		}

		// id
		s32 ID;
		SVarGroupList *VarGroup; // reference

		// Shader: shader name ( also first variable in first Vargroup )
		// Entity: classname ( variable in Group(1) )
		core::stringc name;
	};

	typedef IShader IEntity;

	typedef core::array < IEntity > tQ3EntityList;

	/*
		dump shader like original layout, regardless of internal data holding
		no recursive folding..
	*/
	inline void dumpVarGroup ( core::stringc &dest, const SVarGroup * group, s32 stack )
	{
		core::stringc buf;
		s32 i;


		if ( stack > 0 )
		{
			buf = "";
			for ( i = 0; i < stack - 1; ++i )
				buf += '\t';

			buf += "{\n";
			dest.append ( buf );
		}

		for ( u32 g = 0; g != group->Variable.size(); ++g )
		{
			buf = "";
			for ( i = 0; i < stack; ++i )
				buf += '\t';

			buf += group->Variable[g].name;
			buf += " ";
			buf += group->Variable[g].content;
			buf += "\n";
			dest.append ( buf );
		}

		if ( stack > 1 )
		{
			buf = "";
			for ( i = 0; i < stack - 1; ++i )
				buf += '\t';

			buf += "}\n";
			dest.append ( buf );
		}

	}

	/*!
		dump a Shader or an Entity
	*/
	inline core::stringc & dumpShader ( core::stringc &dest, const IShader * shader, bool entity = false )
	{
		if ( 0 == shader )
			return dest;

		const SVarGroup * group;

		const u32 size = shader->VarGroup->VariableGroup.size ();
		for ( u32 i = 0; i != size; ++i )
		{
			group = &shader->VarGroup->VariableGroup[ i ];
			dumpVarGroup ( dest, group, core::clamp( (int)i, 0, 2 ) );
		}

		if ( !entity )
		{
			if ( size <= 1 )
			{
				dest.append ( "{\n" );
			}
			dest.append ( "}\n" );
		}
		return dest;
	}


	/*
		quake3 doesn't care much about tga & jpg
		load one or multiple files stored in name started at startPos to the texture array textures
		if texture is not loaded 0 will be added ( to find missing textures easier)
	*/
	inline void getTextures(tTexArray &textures,
				const core::stringc &name, u32 &startPos,
				io::IFileSystem *fileSystem,
				video::IVideoDriver* driver)
	{
		static const char* extension[] =
		{
			".jpg",
			".jpeg",
			".png",
			".dds",
			".tga",
			".bmp",
			".pcx"
		};

		tStringList stringList;
		getAsStringList(stringList, -1, name, startPos);

		textures.clear();

		io::path loadFile;
		for ( u32 i = 0; i!= stringList.size (); ++i )
		{
			video::ITexture* texture = 0;
			for (u32 g = 0; g != 7 ; ++g)
			{
				core::cutFilenameExtension ( loadFile, stringList[i] );

				if ( loadFile == "$whiteimage" )
				{
					texture = driver->getTexture( "$whiteimage" );
					if ( 0 == texture )
					{
						core::dimension2du s ( 2, 2 );
						u32 image[4] = { 0xFFFFFFFF, 0xFFFFFFFF,0xFFFFFFFF,0xFFFFFFFF };
						video::IImage* w = driver->createImageFromData ( video::ECF_A8R8G8B8, s,&image );
						texture = driver->addTexture( "$whiteimage", w );
						w->drop ();
					}

				}
				else
				if ( loadFile == "$redimage" )
				{
					texture = driver->getTexture( "$redimage" );
					if ( 0 == texture )
					{
						core::dimension2du s ( 2, 2 );
						u32 image[4] = { 0xFFFF0000, 0xFFFF0000,0xFFFF0000,0xFFFF0000 };
						video::IImage* w = driver->createImageFromData ( video::ECF_A8R8G8B8, s,&image );
						texture = driver->addTexture( "$redimage", w );
						w->drop ();
					}
				}
				else
				if ( loadFile == "$blueimage" )
				{
					texture = driver->getTexture( "$blueimage" );
					if ( 0 == texture )
					{
						core::dimension2du s ( 2, 2 );
						u32 image[4] = { 0xFF0000FF, 0xFF0000FF,0xFF0000FF,0xFF0000FF };
						video::IImage* w = driver->createImageFromData ( video::ECF_A8R8G8B8, s,&image );
						texture = driver->addTexture( "$blueimage", w );
						w->drop ();
					}
				}
				else
				if ( loadFile == "$checkerimage" )
				{
					texture = driver->getTexture( "$checkerimage" );
					if ( 0 == texture )
					{
						core::dimension2du s ( 2, 2 );
						u32 image[4] = { 0xFFFFFFFF, 0xFF000000,0xFF000000,0xFFFFFFFF };
						video::IImage* w = driver->createImageFromData ( video::ECF_A8R8G8B8, s,&image );
						texture = driver->addTexture( "$checkerimage", w );
						w->drop ();
					}
				}
				else
				if ( loadFile == "$lightmap" )
				{
					texture = 0;
				}
				else
				{
					loadFile.append ( extension[g] );
				}

				if ( fileSystem->existFile ( loadFile ) )
				{
					texture = driver->getTexture( loadFile );
					if ( texture )
						break;
					texture = 0;
				}
			}
			// take 0 Texture
			textures.push_back(texture);
		}
	}


	//! Manages various Quake3 Shader Styles
	class IShaderManager : public IReferenceCounted
	{
	};

} // end namespace quake3
} // end namespace scene
} // end namespace irr

#endif

