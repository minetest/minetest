// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_BSP_LOADER_

#include "CQ3LevelMesh.h"
#include "ISceneManager.h"
#include "os.h"
#include "SMeshBufferLightMap.h"
#include "irrString.h"
#include "ILightSceneNode.h"
#include "IQ3Shader.h"
#include "IFileList.h"

//#define TJUNCTION_SOLVER_ROUND
//#define TJUNCTION_SOLVER_0125

namespace irr
{
namespace scene
{

	using namespace quake3;

//! constructor
CQ3LevelMesh::CQ3LevelMesh(io::IFileSystem* fs, scene::ISceneManager* smgr,
				const Q3LevelLoadParameter &loadParam)
	: LoadParam(loadParam), Textures(0), NumTextures(0), LightMaps(0), NumLightMaps(0),
	Vertices(0), NumVertices(0), Faces(0), NumFaces(0), Models(0), NumModels(0),
	Planes(0), NumPlanes(0), Nodes(0), NumNodes(0), Leafs(0), NumLeafs(0),
	LeafFaces(0), NumLeafFaces(0), MeshVerts(0), NumMeshVerts(0),
	Brushes(0), NumBrushes(0), BrushEntities(0), FileSystem(fs),
	SceneManager(smgr), FramesPerSecond(25.f)
{
	#ifdef _DEBUG
	IReferenceCounted::setDebugName("CQ3LevelMesh");
	#endif

	for ( s32 i = 0; i!= E_Q3_MESH_SIZE; ++i )
	{
		Mesh[i] = 0;
	}

	Driver = smgr ? smgr->getVideoDriver() : 0;
	if (Driver)
		Driver->grab();

	if (FileSystem)
		FileSystem->grab();

	// load default shaders
	InitShader();
}


//! destructor
CQ3LevelMesh::~CQ3LevelMesh()
{
	cleanLoader ();

	if (Driver)
		Driver->drop();

	if (FileSystem)
		FileSystem->drop();

	s32 i;

	for ( i = 0; i!= E_Q3_MESH_SIZE; ++i )
	{
		if ( Mesh[i] )
		{
			Mesh[i]->drop();
			Mesh[i] = 0;
		}
	}

	for ( i = 1; i < NumModels; i++ )
	{
		BrushEntities[i]->drop();
	}
	delete [] BrushEntities; BrushEntities = 0;

	ReleaseShader();
	ReleaseEntity();
}


//! loads a level from a .bsp-File. Also tries to load all needed textures. Returns true if successful.
bool CQ3LevelMesh::loadFile(io::IReadFile* file)
{
	if (!file)
		return false;

	LevelName = file->getFileName();

	file->read(&header, sizeof(tBSPHeader));

	#ifdef __BIG_ENDIAN__
		header.strID = os::Byteswap::byteswap(header.strID);
		header.version = os::Byteswap::byteswap(header.version);
	#endif

	if (	(header.strID != 0x50534249 ||			// IBSP
				(	header.version != 0x2e			// quake3
				&& header.version != 0x2f			// rtcw
				)
			)
			&&
			( header.strID != 0x50534252 || header.version != 1 ) // RBSP, starwars jedi, sof
		)
	{
		os::Printer::log("Could not load .bsp file, unknown header.", file->getFileName(), ELL_ERROR);
		return false;
	}

#if 0
	if ( header.strID == 0x50534252 )	// RBSP Raven
	{
		LoadParam.swapHeader = 1;
	}
#endif

	// now read lumps
	file->read(&Lumps[0], sizeof(tBSPLump)*kMaxLumps);

	s32 i;
	if ( LoadParam.swapHeader )
	{
		for ( i=0; i< kMaxLumps;++i)
		{
			Lumps[i].offset = os::Byteswap::byteswap(Lumps[i].offset);
			Lumps[i].length = os::Byteswap::byteswap(Lumps[i].length);
		}
	}

	ReleaseEntity();

	// load everything
	loadEntities(&Lumps[kEntities], file);			// load the entities
	loadTextures(&Lumps[kShaders], file);			// Load the textures
	loadLightmaps(&Lumps[kLightmaps], file);		// Load the lightmaps
	loadVerts(&Lumps[kVertices], file);				// Load the vertices
	loadFaces(&Lumps[kFaces], file);				// Load the faces
	loadPlanes(&Lumps[kPlanes], file);				// Load the Planes of the BSP
	loadNodes(&Lumps[kNodes], file);				// load the Nodes of the BSP
	loadLeafs(&Lumps[kLeafs], file);				// load the Leafs of the BSP
	loadLeafFaces(&Lumps[kLeafFaces], file);		// load the Faces of the Leafs of the BSP
	loadVisData(&Lumps[kVisData], file);			// load the visibility data of the clusters
	loadModels(&Lumps[kModels], file);				// load the models
	loadMeshVerts(&Lumps[kMeshVerts], file);		// load the mesh vertices
	loadBrushes(&Lumps[kBrushes], file);			// load the brushes of the BSP
	loadBrushSides(&Lumps[kBrushSides], file);		// load the brushsides of the BSP
	loadLeafBrushes(&Lumps[kLeafBrushes], file);	// load the brushes of the leaf
	loadFogs(&Lumps[kFogs], file );					// load the fogs

	loadTextures();
	constructMesh();
	solveTJunction();

	cleanMeshes();
	calcBoundingBoxes();
	cleanLoader();

	return true;
}

/*!
*/
void CQ3LevelMesh::cleanLoader ()
{
	delete [] Textures; Textures = 0;
	delete [] LightMaps; LightMaps = 0;
	delete [] Vertices; Vertices = 0;
	delete [] Faces; Faces = 0;
	delete [] Models; Models = 0;
	delete [] Planes; Planes = 0;
	delete [] Nodes; Nodes = 0;
	delete [] Leafs; Leafs = 0;
	delete [] LeafFaces; LeafFaces = 0;
	delete [] MeshVerts; MeshVerts = 0;
	delete [] Brushes; Brushes = 0;

	Lightmap.clear();
	Tex.clear();
}

//! returns the amount of frames in milliseconds. If the amount is 1, it is a static (=non animated) mesh.
u32 CQ3LevelMesh::getFrameCount() const
{
	return 1;
}


//! returns the animated mesh based on a detail level. 0 is the lowest, 255 the highest detail. Note, that some Meshes will ignore the detail level.
IMesh* CQ3LevelMesh::getMesh(s32 frameInMs, s32 detailLevel, s32 startFrameLoop, s32 endFrameLoop)
{
	return Mesh[frameInMs];
}


void CQ3LevelMesh::loadTextures(tBSPLump* l, io::IReadFile* file)
{
	NumTextures = l->length / sizeof(tBSPTexture);
	if ( !NumTextures )
		return;
	Textures = new tBSPTexture[NumTextures];

	file->seek(l->offset);
	file->read(Textures, l->length);

	if ( LoadParam.swapHeader )
	{
		for (s32 i=0;i<NumTextures;++i)
		{
			Textures[i].flags = os::Byteswap::byteswap(Textures[i].flags);
			Textures[i].contents = os::Byteswap::byteswap(Textures[i].contents);
			//os::Printer::log("Loaded texture", Textures[i].strName, ELL_INFORMATION);
		}
	}
}


void CQ3LevelMesh::loadLightmaps(tBSPLump* l, io::IReadFile* file)
{
	NumLightMaps = l->length / sizeof(tBSPLightmap);
	if ( !NumLightMaps )
		return;
	LightMaps = new tBSPLightmap[NumLightMaps];

	file->seek(l->offset);
	file->read(LightMaps, l->length);
}

/*!
*/
void CQ3LevelMesh::loadVerts(tBSPLump* l, io::IReadFile* file)
{
	NumVertices = l->length / sizeof(tBSPVertex);
	if ( !NumVertices )
		return;
	Vertices = new tBSPVertex[NumVertices];

	file->seek(l->offset);
	file->read(Vertices, l->length);

	if ( LoadParam.swapHeader )
	for (s32 i=0;i<NumVertices;i++)
	{
		Vertices[i].vPosition[0] = os::Byteswap::byteswap(Vertices[i].vPosition[0]);
		Vertices[i].vPosition[1] = os::Byteswap::byteswap(Vertices[i].vPosition[1]);
		Vertices[i].vPosition[2] = os::Byteswap::byteswap(Vertices[i].vPosition[2]);
		Vertices[i].vTextureCoord[0] = os::Byteswap::byteswap(Vertices[i].vTextureCoord[0]);
		Vertices[i].vTextureCoord[1] = os::Byteswap::byteswap(Vertices[i].vTextureCoord[1]);
		Vertices[i].vLightmapCoord[0] = os::Byteswap::byteswap(Vertices[i].vLightmapCoord[0]);
		Vertices[i].vLightmapCoord[1] = os::Byteswap::byteswap(Vertices[i].vLightmapCoord[1]);
		Vertices[i].vNormal[0] = os::Byteswap::byteswap(Vertices[i].vNormal[0]);
		Vertices[i].vNormal[1] = os::Byteswap::byteswap(Vertices[i].vNormal[1]);
		Vertices[i].vNormal[2] = os::Byteswap::byteswap(Vertices[i].vNormal[2]);
	}
}


/*!
*/
void CQ3LevelMesh::loadFaces(tBSPLump* l, io::IReadFile* file)
{
	NumFaces = l->length / sizeof(tBSPFace);
	if (!NumFaces)
		return;
	Faces = new tBSPFace[NumFaces];

	file->seek(l->offset);
	file->read(Faces, l->length);

	if ( LoadParam.swapHeader )
	{
		for ( s32 i=0;i<NumFaces;i++)
		{
			Faces[i].textureID = os::Byteswap::byteswap(Faces[i].textureID);
			Faces[i].fogNum = os::Byteswap::byteswap(Faces[i].fogNum);
			Faces[i].type = os::Byteswap::byteswap(Faces[i].type);
			Faces[i].vertexIndex = os::Byteswap::byteswap(Faces[i].vertexIndex);
			Faces[i].numOfVerts = os::Byteswap::byteswap(Faces[i].numOfVerts);
			Faces[i].meshVertIndex = os::Byteswap::byteswap(Faces[i].meshVertIndex);
			Faces[i].numMeshVerts = os::Byteswap::byteswap(Faces[i].numMeshVerts);
			Faces[i].lightmapID = os::Byteswap::byteswap(Faces[i].lightmapID);
			Faces[i].lMapCorner[0] = os::Byteswap::byteswap(Faces[i].lMapCorner[0]);
			Faces[i].lMapCorner[1] = os::Byteswap::byteswap(Faces[i].lMapCorner[1]);
			Faces[i].lMapSize[0] = os::Byteswap::byteswap(Faces[i].lMapSize[0]);
			Faces[i].lMapSize[1] = os::Byteswap::byteswap(Faces[i].lMapSize[1]);
			Faces[i].lMapPos[0] = os::Byteswap::byteswap(Faces[i].lMapPos[0]);
			Faces[i].lMapPos[1] = os::Byteswap::byteswap(Faces[i].lMapPos[1]);
			Faces[i].lMapPos[2] = os::Byteswap::byteswap(Faces[i].lMapPos[2]);
			Faces[i].lMapBitsets[0][0] = os::Byteswap::byteswap(Faces[i].lMapBitsets[0][0]);
			Faces[i].lMapBitsets[0][1] = os::Byteswap::byteswap(Faces[i].lMapBitsets[0][1]);
			Faces[i].lMapBitsets[0][2] = os::Byteswap::byteswap(Faces[i].lMapBitsets[0][2]);
			Faces[i].lMapBitsets[1][0] = os::Byteswap::byteswap(Faces[i].lMapBitsets[1][0]);
			Faces[i].lMapBitsets[1][1] = os::Byteswap::byteswap(Faces[i].lMapBitsets[1][1]);
			Faces[i].lMapBitsets[1][2] = os::Byteswap::byteswap(Faces[i].lMapBitsets[1][2]);
			Faces[i].vNormal[0] = os::Byteswap::byteswap(Faces[i].vNormal[0]);
			Faces[i].vNormal[1] = os::Byteswap::byteswap(Faces[i].vNormal[1]);
			Faces[i].vNormal[2] = os::Byteswap::byteswap(Faces[i].vNormal[2]);
			Faces[i].size[0] = os::Byteswap::byteswap(Faces[i].size[0]);
			Faces[i].size[1] = os::Byteswap::byteswap(Faces[i].size[1]);
		}
	}
}


/*!
*/
void CQ3LevelMesh::loadPlanes(tBSPLump* l, io::IReadFile* file)
{
	// ignore
}


/*!
*/
void CQ3LevelMesh::loadNodes(tBSPLump* l, io::IReadFile* file)
{
	// ignore
}


/*!
*/
void CQ3LevelMesh::loadLeafs(tBSPLump* l, io::IReadFile* file)
{
	// ignore
}


/*!
*/
void CQ3LevelMesh::loadLeafFaces(tBSPLump* l, io::IReadFile* file)
{
	// ignore
}


/*!
*/
void CQ3LevelMesh::loadVisData(tBSPLump* l, io::IReadFile* file)
{
	// ignore
}


/*!
*/
void CQ3LevelMesh::loadEntities(tBSPLump* l, io::IReadFile* file)
{
	core::array<u8> entity;
	entity.set_used( l->length + 2 );
	entity[l->length + 1 ] = 0;

	file->seek(l->offset);
	file->read( entity.pointer(), l->length);

	parser_parse( entity.pointer(), l->length, &CQ3LevelMesh::scriptcallback_entity );
}


/*!
	load fog brushes
*/
void CQ3LevelMesh::loadFogs(tBSPLump* l, io::IReadFile* file)
{
	u32 files = l->length / sizeof(tBSPFog);

	file->seek( l->offset );
	tBSPFog fog;
	const IShader *shader;
	STexShader t;
	for ( u32 i = 0; i!= files; ++i )
	{
		file->read( &fog, sizeof( fog ) );

		shader = getShader( fog.shader );
		t.Texture = 0;
		t.ShaderID = shader ? shader->ID : -1;

		FogMap.push_back ( t );
	}
}


/*!
	load models named in bsp
*/
void CQ3LevelMesh::loadModels(tBSPLump* l, io::IReadFile* file)
{
	NumModels = l->length / sizeof(tBSPModel);
	Models = new tBSPModel[NumModels];

	file->seek( l->offset );
	file->read(Models, l->length);

	if ( LoadParam.swapHeader )
	{
		for ( s32 i = 0; i < NumModels; i++)
		{
			Models[i].min[0] = os::Byteswap::byteswap(Models[i].min[0]);
			Models[i].min[1] = os::Byteswap::byteswap(Models[i].min[1]);
			Models[i].min[2] = os::Byteswap::byteswap(Models[i].min[2]);
			Models[i].max[0] = os::Byteswap::byteswap(Models[i].max[0]);
			Models[i].max[1] = os::Byteswap::byteswap(Models[i].max[1]);
			Models[i].max[2] = os::Byteswap::byteswap(Models[i].max[2]);

			Models[i].faceIndex = os::Byteswap::byteswap(Models[i].faceIndex);
			Models[i].numOfFaces = os::Byteswap::byteswap(Models[i].numOfFaces);
			Models[i].brushIndex = os::Byteswap::byteswap(Models[i].brushIndex);
			Models[i].numOfBrushes = os::Byteswap::byteswap(Models[i].numOfBrushes);
		}
	}

	BrushEntities = new SMesh*[NumModels];
}

/*!
*/
void CQ3LevelMesh::loadMeshVerts(tBSPLump* l, io::IReadFile* file)
{
	NumMeshVerts = l->length / sizeof(s32);
	if (!NumMeshVerts)
		return;
	MeshVerts = new s32[NumMeshVerts];

	file->seek(l->offset);
	file->read(MeshVerts, l->length);

	if ( LoadParam.swapHeader )
	{
		for (int i=0;i<NumMeshVerts;i++)
			MeshVerts[i] = os::Byteswap::byteswap(MeshVerts[i]);
	}
}

/*!
*/
void CQ3LevelMesh::loadBrushes(tBSPLump* l, io::IReadFile* file)
{
	// ignore
}

/*!
*/
void CQ3LevelMesh::loadBrushSides(tBSPLump* l, io::IReadFile* file)
{
	// ignore
}

/*!
*/
void CQ3LevelMesh::loadLeafBrushes(tBSPLump* l, io::IReadFile* file)
{
	// ignore
}

/*!
*/
inline bool isQ3WhiteSpace( const u8 symbol )
{
	return symbol == ' ' || symbol == '\t' || symbol == '\r';
}

/*!
*/
inline bool isQ3ValidName( const u8 symbol )
{
	return	(symbol >= 'a' && symbol <= 'z' ) ||
			(symbol >= 'A' && symbol <= 'Z' ) ||
			(symbol >= '0' && symbol <= '9' ) ||
			(symbol == '/' || symbol == '_' || symbol == '.' );
}

/*!
*/
void CQ3LevelMesh::parser_nextToken()
{
	u8 symbol;

	Parser.token = "";
	Parser.tokenresult = Q3_TOKEN_UNRESOLVED;

	// skip white space
	do
	{
		if ( Parser.index >= Parser.sourcesize )
		{
			Parser.tokenresult = Q3_TOKEN_EOF;
			return;
		}

		symbol = Parser.source [ Parser.index ];
		Parser.index += 1;
	} while ( isQ3WhiteSpace( symbol ) );

	// first symbol, one symbol
	switch ( symbol )
	{
		case 0:
			Parser.tokenresult = Q3_TOKEN_EOF;
			return;

		case '/':
			// comment or divide
			if ( Parser.index >= Parser.sourcesize )
			{
				Parser.tokenresult = Q3_TOKEN_EOF;
				return;
			}
			symbol = Parser.source [ Parser.index ];
			Parser.index += 1;
			if ( isQ3WhiteSpace( symbol ) )
			{
				Parser.tokenresult = Q3_TOKEN_MATH_DIVIDE;
				return;
			}
			else
			if ( symbol == '*' )
			{
				// C-style comment in quake?
			}
			else
			if ( symbol == '/' )
			{
				// skip to eol
				do
				{
					if ( Parser.index >= Parser.sourcesize )
					{
						Parser.tokenresult = Q3_TOKEN_EOF;
						return;
					}
					symbol = Parser.source [ Parser.index ];
					Parser.index += 1;
				} while ( symbol != '\n' );
				Parser.tokenresult = Q3_TOKEN_COMMENT;
				return;
			}
			// take /[name] as valid token..?!?!?. mhmm, maybe
			break;

		case '\n':
			Parser.tokenresult = Q3_TOKEN_EOL;
			return;
		case '{':
			Parser.tokenresult = Q3_TOKEN_START_LIST;
			return;
		case '}':
			Parser.tokenresult = Q3_TOKEN_END_LIST;
			return;

		case '"':
			// string literal
			do
			{
				if ( Parser.index >= Parser.sourcesize )
				{
					Parser.tokenresult = Q3_TOKEN_EOF;
					return;
				}
				symbol = Parser.source [ Parser.index ];
				Parser.index += 1;
				if ( symbol != '"' )
					Parser.token.append( symbol );
			} while ( symbol != '"' );
			Parser.tokenresult = Q3_TOKEN_ENTITY;
			return;
	}

	// user identity
	Parser.token.append( symbol );

	// continue till whitespace
	bool validName = true;
	do
	{
		if ( Parser.index >= Parser.sourcesize )
		{
			Parser.tokenresult = Q3_TOKEN_EOF;
			return;
		}
		symbol = Parser.source [ Parser.index ];

		validName = isQ3ValidName( symbol );
		if ( validName )
		{
			Parser.token.append( symbol );
			Parser.index += 1;
		}
	} while ( validName );

	Parser.tokenresult = Q3_TOKEN_TOKEN;
	return;
}


/*
	parse entity & shader
	calls callback on content in {}
*/
void CQ3LevelMesh::parser_parse( const void * data, const u32 size, CQ3LevelMesh::tParserCallback callback )
{
	Parser.source = static_cast<const c8*>(data);
	Parser.sourcesize = size;
	Parser.index = 0;

	SVarGroupList *groupList;

	s32 active;
	s32 last;

	SVariable entity ( "" );

	groupList = new SVarGroupList();

	groupList->VariableGroup.push_back( SVarGroup() );
	active = last = 0;

	do
	{
		parser_nextToken();

		switch ( Parser.tokenresult )
		{
			case Q3_TOKEN_START_LIST:
			{
				//stack = core::min_( stack + 1, 7 );

				groupList->VariableGroup.push_back( SVarGroup() );
				last = active;
				active = groupList->VariableGroup.size() - 1;
				entity.clear();
			}  break;

			// a unregisterd variable is finished
			case Q3_TOKEN_EOL:
			{
				if ( entity.isValid() )
				{
					groupList->VariableGroup[active].Variable.push_back( entity );
					entity.clear();
				}
			} break;

			case Q3_TOKEN_TOKEN:
			case Q3_TOKEN_ENTITY:
			{
				Parser.token.make_lower();

				// store content based on line-delemiter
				if ( 0 == entity.isValid() )
				{
					entity.name = Parser.token;
					entity.content = "";

				}
				else
				{
					if ( entity.content.size() )
					{
						entity.content += " ";
					}
					entity.content += Parser.token;
				}
			} break;

			case Q3_TOKEN_END_LIST:
			{
				//stack = core::max_( stack - 1, 0 );

				// close tag for first
				if ( active == 1 )
				{
					(this->*callback)( groupList, Q3_TOKEN_END_LIST );

					// new group
					groupList->drop();
					groupList = new SVarGroupList();
					groupList->VariableGroup.push_back( SVarGroup() );
					last = 0;
				}

				active = last;
				entity.clear();

			} break;

			default:
			break;
		}

	} while ( Parser.tokenresult != Q3_TOKEN_EOF );

	(this->*callback)( groupList, Q3_TOKEN_EOF );

	groupList->drop();
}


/*
	this loader applies only textures for stage 1 & 2
*/
s32 CQ3LevelMesh::setShaderFogMaterial( video::SMaterial &material, const tBSPFace * face ) const
{
	material.MaterialType = video::EMT_SOLID;
	material.Wireframe = false;
	material.Lighting = false;
	material.BackfaceCulling = false;
	material.setTexture(0, 0);
	material.setTexture(1, 0);
	material.setTexture(2, 0);
	material.setTexture(3, 0);
	material.ZBuffer = video::ECFN_LESSEQUAL;
	material.ZWriteEnable = false;
	material.MaterialTypeParam = 0.f;

	s32 shaderState = -1;

	if ( (u32) face->fogNum < FogMap.size() )
	{
		material.setTexture(0, FogMap [ face->fogNum ].Texture);
		shaderState = FogMap [ face->fogNum ].ShaderID;
	}

	return shaderState;

}
/*
	this loader applies only textures for stage 1 & 2
*/
s32 CQ3LevelMesh::setShaderMaterial( video::SMaterial &material, const tBSPFace * face ) const
{
	material.MaterialType = video::EMT_SOLID;
	material.Wireframe = false;
	material.Lighting = false;
	material.BackfaceCulling = true;
	material.setTexture(0, 0);
	material.setTexture(1, 0);
	material.setTexture(2, 0);
	material.setTexture(3, 0);
	material.ZBuffer = video::ECFN_LESSEQUAL;
	material.ZWriteEnable = true;
	material.MaterialTypeParam = 0.f;

	s32 shaderState = -1;

	if ( face->textureID >= 0 && face->textureID < (s32)Tex.size() )
	{
		material.setTexture(0, Tex [ face->textureID ].Texture);
		shaderState = Tex [ face->textureID ].ShaderID;
	}

	if ( face->lightmapID >= 0 && face->lightmapID < (s32)Lightmap.size() )
	{
		material.setTexture(1, Lightmap [ face->lightmapID ]);
		material.MaterialType = LoadParam.defaultLightMapMaterial;
	}

	// store shader ID
	material.MaterialTypeParam2 = (f32) shaderState;

	const IShader *shader = getShader(shaderState);
	if ( 0 == shader )
		return shaderState;

	return shaderState;

#if 0
	const SVarGroup *group;


	// generic
	group = shader->getGroup( 1 );
	if ( group )
	{
		material.BackfaceCulling = getCullingFunction( group->get( "cull" ) );

		if ( group->isDefined( "surfaceparm", "nolightmap" ) )
		{
			material.MaterialType = video::EMT_SOLID;
			material.setTexture(1, 0);
		}

	}

	// try to get the best of the 8 texture stages..

	// texture 1, texture 2
	u32 startPos;
	for ( s32 g = 2; g <= 3; ++g )
	{
		group = shader->getGroup( g );
		if ( 0 == group )
			continue;

		startPos = 0;

		if ( group->isDefined( "depthwrite" ) )
		{
			material.ZWriteEnable = true;
		}

		SBlendFunc blendfunc ( LoadParam.defaultModulate );
		getBlendFunc( group->get( "blendfunc" ), blendfunc );
		getBlendFunc( group->get( "alphafunc" ), blendfunc );

		if ( 0 == LoadParam.alpharef &&
			(	blendfunc.type == video::EMT_TRANSPARENT_ALPHA_CHANNEL ||
				blendfunc.type == video::EMT_TRANSPARENT_ALPHA_CHANNEL_REF
			)
			)
		{
			blendfunc.type = video::EMT_TRANSPARENT_ALPHA_CHANNEL;
			blendfunc.param0 = 0.f;
		}

		material.MaterialType = blendfunc.type;
		material.MaterialTypeParam = blendfunc.param0;

		// try if we can match better
		shaderState |= (material.MaterialType == video::EMT_SOLID ) ? 0x00020000 : 0;
	}

	//material.BackfaceCulling = false;

	if ( shader->VarGroup->VariableGroup.size() <= 4 )
	{
		shaderState |= 0x00010000;
	}

	material.MaterialTypeParam2 = (f32) shaderState;
	return shaderState;
#endif
}

/*!
	Internal function to build a mesh.
*/
scene::SMesh** CQ3LevelMesh::buildMesh(s32 num)
{
	scene::SMesh** newmesh = new SMesh *[quake3::E_Q3_MESH_SIZE];

	s32 i, j, k,s;

	for (i = 0; i < E_Q3_MESH_SIZE; i++)
	{
		newmesh[i] = new SMesh();
	}

	s32 *index;

	video::S3DVertex2TCoords temp[3];
	video::SMaterial material;
	video::SMaterial material2;

	SToBuffer item [ E_Q3_MESH_SIZE ];
	u32 itemSize;

	for (i = Models[num].faceIndex; i < Models[num].numOfFaces + Models[num].faceIndex; ++i)
	{
		const tBSPFace * face = Faces + i;

		s32 shaderState = setShaderMaterial( material, face );
		itemSize = 0;

		const IShader *shader = getShader(shaderState);

		if ( face->fogNum >= 0 )
		{
			setShaderFogMaterial ( material2, face );
			item[itemSize].index = E_Q3_MESH_FOG;
			item[itemSize].takeVertexColor = 1;
			itemSize += 1;
		}

		switch( face->type )
		{
			case 1: // normal polygons
			case 2: // patches
			case 3: // meshes
				if ( 0 == shader )
				{
					if ( LoadParam.cleanUnResolvedMeshes || material.getTexture(0) )
					{
						item[itemSize].takeVertexColor = 1;
						item[itemSize].index = E_Q3_MESH_GEOMETRY;
						itemSize += 1;
					}
					else
					{
						item[itemSize].takeVertexColor = 1;
						item[itemSize].index = E_Q3_MESH_UNRESOLVED;
						itemSize += 1;
					}
				}
				else
				{
					item[itemSize].takeVertexColor = 1;
					item[itemSize].index = E_Q3_MESH_ITEMS;
					itemSize += 1;
				}
			break;

			case 4: // billboards
				//item[itemSize].takeVertexColor = 1;
				//item[itemSize].index = E_Q3_MESH_ITEMS;
				//itemSize += 1;
			break;

		}

		for ( u32 g = 0; g != itemSize; ++g )
		{
			scene::SMeshBufferLightMap* buffer = 0;

			if ( item[g].index == E_Q3_MESH_GEOMETRY )
			{
				if ( 0 == item[g].takeVertexColor )
				{
					item[g].takeVertexColor = material.getTexture(0) == 0 || material.getTexture(1) == 0;
				}

				if (Faces[i].lightmapID < -1 || Faces[i].lightmapID > NumLightMaps-1)
				{
					Faces[i].lightmapID = -1;
				}

#if 0
				// there are lightmapsids and textureid with -1
				const s32 tmp_index = ((Faces[i].lightmapID+1) * (NumTextures+1)) + (Faces[i].textureID+1);
				buffer = (SMeshBufferLightMap*) newmesh[E_Q3_MESH_GEOMETRY]->getMeshBuffer(tmp_index);
				buffer->setHardwareMappingHint ( EHM_STATIC );
				buffer->getMaterial() = material;
#endif
			}

			// Construct a unique mesh for each shader or combine meshbuffers for same shader
			if ( 0 == buffer )
			{

				if ( LoadParam.mergeShaderBuffer == 1 )
				{
					// combine
					buffer = (SMeshBufferLightMap*) newmesh[ item[g].index ]->getMeshBuffer(
						item[g].index != E_Q3_MESH_FOG ? material : material2 );
				}

				// create a seperate mesh buffer
				if ( 0 == buffer )
				{
					buffer = new scene::SMeshBufferLightMap();
					newmesh[ item[g].index ]->addMeshBuffer( buffer );
					buffer->drop();
					buffer->getMaterial() = item[g].index != E_Q3_MESH_FOG ? material : material2;
					if ( item[g].index == E_Q3_MESH_GEOMETRY )
						buffer->setHardwareMappingHint ( EHM_STATIC );
				}
			}


			switch(Faces[i].type)
			{
				case 4: // billboards
					break;
				case 2: // patches
					createCurvedSurface_bezier( buffer, i,
									LoadParam.patchTesselation,
									item[g].takeVertexColor
								  );
					break;

				case 1: // normal polygons
				case 3: // mesh vertices
					index = MeshVerts + face->meshVertIndex;
					k = buffer->getVertexCount();

					// reallocate better if many small meshes are used
					s = buffer->getIndexCount()+face->numMeshVerts;
					if ( buffer->Indices.allocated_size () < (u32) s )
					{
						if (	buffer->Indices.allocated_size () > 0 &&
								face->numMeshVerts < 20 && NumFaces > 1000
							)
						{
							s = buffer->getIndexCount() + (NumFaces >> 3 * face->numMeshVerts );
						}
						buffer->Indices.reallocate( s);
					}

					for ( j = 0; j < face->numMeshVerts; ++j )
					{
						buffer->Indices.push_back( k + index [j] );
					}

					s = k+face->numOfVerts;
					if ( buffer->Vertices.allocated_size () < (u32) s )
					{
						if (	buffer->Indices.allocated_size () > 0 &&
								face->numOfVerts < 20 && NumFaces > 1000
							)
						{
							s = buffer->getIndexCount() + (NumFaces >> 3 * face->numOfVerts );
						}
						buffer->Vertices.reallocate( s);
					}
					for ( j = 0; j != face->numOfVerts; ++j )
					{
						copy( &temp[0], &Vertices[ j + face->vertexIndex ], item[g].takeVertexColor );
						buffer->Vertices.push_back( temp[0] );
					}
					break;

			} // end switch
		}
	}

	return newmesh;
}

/*!
*/
void CQ3LevelMesh::solveTJunction()
{
}

/*!
	constructs a mesh from the quake 3 level file.
*/
void CQ3LevelMesh::constructMesh()
{
	if ( LoadParam.verbose > 0 )
	{
		LoadParam.startTime = os::Timer::getRealTime();

		if ( LoadParam.verbose > 1 )
		{
			snprintf( buf, sizeof ( buf ),
				"quake3::constructMesh start to create %d faces, %d vertices,%d mesh vertices",
				NumFaces,
				NumVertices,
				NumMeshVerts
				);
			os::Printer::log(buf, ELL_INFORMATION);
		}

	}

	s32 i, j;

	// First the main level
	SMesh **tmp = buildMesh(0);

	for (i = 0; i < E_Q3_MESH_SIZE; i++)
	{
		Mesh[i] = tmp[i];
	}
	delete [] tmp;

	// Then the brush entities

	for (i = 1; i < NumModels; i++)
	{
		tmp = buildMesh(i);
		BrushEntities[i] = tmp[0];

		// We only care about the main geometry here
		for (j = 1; j < E_Q3_MESH_SIZE; j++)
		{
			tmp[j]->drop();
		}
		delete [] tmp;
	}

	if ( LoadParam.verbose > 0 )
	{
		LoadParam.endTime = os::Timer::getRealTime();

		snprintf( buf, sizeof ( buf ),
			"quake3::constructMesh needed %04d ms to create %d faces, %d vertices,%d mesh vertices",
			LoadParam.endTime - LoadParam.startTime,
			NumFaces,
			NumVertices,
			NumMeshVerts
			);
		os::Printer::log(buf, ELL_INFORMATION);
	}

}


void CQ3LevelMesh::S3DVertex2TCoords_64::copy( video::S3DVertex2TCoords &dest ) const
{
#if defined (TJUNCTION_SOLVER_ROUND)
	dest.Pos.X = core::round_( (f32) Pos.X );
	dest.Pos.Y = core::round_( (f32) Pos.Y );
	dest.Pos.Z = core::round_( (f32) Pos.Z );
#elif defined (TJUNCTION_SOLVER_0125)
	dest.Pos.X = (f32) ( floor ( Pos.X * 8.f + 0.5 ) * 0.125 );
	dest.Pos.Y = (f32) ( floor ( Pos.Y * 8.f + 0.5 ) * 0.125 );
	dest.Pos.Z = (f32) ( floor ( Pos.Z * 8.f + 0.5 ) * 0.125 );
#else
	dest.Pos.X = (f32) Pos.X;
	dest.Pos.Y = (f32) Pos.Y;
	dest.Pos.Z = (f32) Pos.Z;
#endif

	dest.Normal.X = (f32) Normal.X;
	dest.Normal.Y = (f32) Normal.Y;
	dest.Normal.Z = (f32) Normal.Z;
	dest.Normal.normalize();

	dest.Color = Color.toSColor();

	dest.TCoords.X = (f32) TCoords.X;
	dest.TCoords.Y = (f32) TCoords.Y;

	dest.TCoords2.X = (f32) TCoords2.X;
	dest.TCoords2.Y = (f32) TCoords2.Y;
}


void CQ3LevelMesh::copy( S3DVertex2TCoords_64 * dest, const tBSPVertex * source, s32 vertexcolor ) const
{
#if defined (TJUNCTION_SOLVER_ROUND)
	dest->Pos.X = core::round_( source->vPosition[0] );
	dest->Pos.Y = core::round_( source->vPosition[2] );
	dest->Pos.Z = core::round_( source->vPosition[1] );
#elif defined (TJUNCTION_SOLVER_0125)
	dest->Pos.X = (f32) ( floor ( source->vPosition[0] * 8.f + 0.5 ) * 0.125 );
	dest->Pos.Y = (f32) ( floor ( source->vPosition[2] * 8.f + 0.5 ) * 0.125 );
	dest->Pos.Z = (f32) ( floor ( source->vPosition[1] * 8.f + 0.5 ) * 0.125 );
#else
	dest->Pos.X = source->vPosition[0];
	dest->Pos.Y = source->vPosition[2];
	dest->Pos.Z = source->vPosition[1];
#endif

	dest->Normal.X = source->vNormal[0];
	dest->Normal.Y = source->vNormal[2];
	dest->Normal.Z = source->vNormal[1];
	dest->Normal.normalize();

	dest->TCoords.X = source->vTextureCoord[0];
	dest->TCoords.Y = source->vTextureCoord[1];
	dest->TCoords2.X = source->vLightmapCoord[0];
	dest->TCoords2.Y = source->vLightmapCoord[1];

	if ( vertexcolor )
	{
		//u32 a = core::s32_min( source->color[3] * LoadParam.defaultModulate, 255 );
		u32 a = source->color[3];
		u32 r = core::s32_min( source->color[0] * LoadParam.defaultModulate, 255 );
		u32 g = core::s32_min( source->color[1] * LoadParam.defaultModulate, 255 );
		u32 b = core::s32_min( source->color[2] * LoadParam.defaultModulate, 255 );

		dest->Color.set(a * 1.f/255.f, r * 1.f/255.f,
				g * 1.f/255.f, b * 1.f/255.f);
	}
	else
	{
		dest->Color.set( 1.f, 1.f, 1.f, 1.f );
	}
}


inline void CQ3LevelMesh::copy( video::S3DVertex2TCoords * dest, const tBSPVertex * source, s32 vertexcolor ) const
{
#if defined (TJUNCTION_SOLVER_ROUND)
	dest->Pos.X = core::round_( source->vPosition[0] );
	dest->Pos.Y = core::round_( source->vPosition[2] );
	dest->Pos.Z = core::round_( source->vPosition[1] );
#elif defined (TJUNCTION_SOLVER_0125)
	dest->Pos.X = (f32) ( floor ( source->vPosition[0] * 8.f + 0.5 ) * 0.125 );
	dest->Pos.Y = (f32) ( floor ( source->vPosition[2] * 8.f + 0.5 ) * 0.125 );
	dest->Pos.Z = (f32) ( floor ( source->vPosition[1] * 8.f + 0.5 ) * 0.125 );
#else
	dest->Pos.X = source->vPosition[0];
	dest->Pos.Y = source->vPosition[2];
	dest->Pos.Z = source->vPosition[1];
#endif

	dest->Normal.X = source->vNormal[0];
	dest->Normal.Y = source->vNormal[2];
	dest->Normal.Z = source->vNormal[1];
	dest->Normal.normalize();

	dest->TCoords.X = source->vTextureCoord[0];
	dest->TCoords.Y = source->vTextureCoord[1];
	dest->TCoords2.X = source->vLightmapCoord[0];
	dest->TCoords2.Y = source->vLightmapCoord[1];

	if ( vertexcolor )
	{
		//u32 a = core::s32_min( source->color[3] * LoadParam.defaultModulate, 255 );
		u32 a = source->color[3];
		u32 r = core::s32_min( source->color[0] * LoadParam.defaultModulate, 255 );
		u32 g = core::s32_min( source->color[1] * LoadParam.defaultModulate, 255 );
		u32 b = core::s32_min( source->color[2] * LoadParam.defaultModulate, 255 );

		dest->Color.set(a << 24 | r << 16 | g << 8 | b);
	}
	else
	{
		dest->Color.set(0xFFFFFFFF);
	}
}


void CQ3LevelMesh::SBezier::tesselate( s32 level )
{
	//Calculate how many vertices across/down there are
	s32 j, k;

	column[0].set_used( level + 1 );
	column[1].set_used( level + 1 );
	column[2].set_used( level + 1 );

	const f64 w = 0.0 + (1.0 / (f64) level );

	//Tesselate along the columns
	for( j = 0; j <= level; ++j)
	{
		const f64 f = w * (f64) j;

		column[0][j] = control[0].getInterpolated_quadratic(control[3], control[6], f );
		column[1][j] = control[1].getInterpolated_quadratic(control[4], control[7], f );
		column[2][j] = control[2].getInterpolated_quadratic(control[5], control[8], f );
	}

	const u32 idx = Patch->Vertices.size();
	Patch->Vertices.reallocate(idx+level*level);
	//Tesselate across the rows to get final vertices
	video::S3DVertex2TCoords v;
	S3DVertex2TCoords_64 f;
	for( j = 0; j <= level; ++j)
	{
		for( k = 0; k <= level; ++k)
		{
			f = column[0][j].getInterpolated_quadratic(column[1][j], column[2][j], w * (f64) k);
			f.copy( v );
			Patch->Vertices.push_back( v );
		}
	}

	Patch->Indices.reallocate(Patch->Indices.size()+6*level*level);
	// connect
	for( j = 0; j < level; ++j)
	{
		for( k = 0; k < level; ++k)
		{
			const s32 inx = idx + ( k * ( level + 1 ) ) + j;

			Patch->Indices.push_back( inx + 0 );
			Patch->Indices.push_back( inx + (level + 1 ) + 0 );
			Patch->Indices.push_back( inx + (level + 1 ) + 1 );

			Patch->Indices.push_back( inx + 0 );
			Patch->Indices.push_back( inx + (level + 1 ) + 1 );
			Patch->Indices.push_back( inx + 1 );
		}
	}
}


/*!
	no subdivision
*/
void CQ3LevelMesh::createCurvedSurface_nosubdivision(SMeshBufferLightMap* meshBuffer,
					s32 faceIndex,
					s32 patchTesselation,
					s32 storevertexcolor)
{
	tBSPFace * face = &Faces[faceIndex];
	u32 j,k,m;

	// number of control points across & up
	const u32 controlWidth = face->size[0];
	const u32 controlHeight = face->size[1];
	if ( 0 == controlWidth || 0 == controlHeight )
		return;

	video::S3DVertex2TCoords v;

	m = meshBuffer->Vertices.size();
	meshBuffer->Vertices.reallocate(m+controlHeight * controlWidth);
	for ( j = 0; j!= controlHeight * controlWidth; ++j )
	{
		copy( &v, &Vertices [ face->vertexIndex + j ], storevertexcolor );
		meshBuffer->Vertices.push_back( v );
	}

	meshBuffer->Indices.reallocate(meshBuffer->Indices.size()+6*(controlHeight-1) * (controlWidth-1));
	for ( j = 0; j!= controlHeight - 1; ++j )
	{
		for ( k = 0; k!= controlWidth - 1; ++k )
		{
			meshBuffer->Indices.push_back( m + k + 0 );
			meshBuffer->Indices.push_back( m + k + controlWidth + 0 );
			meshBuffer->Indices.push_back( m + k + controlWidth + 1 );

			meshBuffer->Indices.push_back( m + k + 0 );
			meshBuffer->Indices.push_back( m + k + controlWidth + 1 );
			meshBuffer->Indices.push_back( m + k + 1 );
		}
		m += controlWidth;
	}
}


/*!
*/
void CQ3LevelMesh::createCurvedSurface_bezier(SMeshBufferLightMap* meshBuffer,
					s32 faceIndex,
					s32 patchTesselation,
					s32 storevertexcolor)
{

	tBSPFace * face = &Faces[faceIndex];
	u32 j,k;

	// number of control points across & up
	const u32 controlWidth = face->size[0];
	const u32 controlHeight = face->size[1];

	if ( 0 == controlWidth || 0 == controlHeight )
		return;

	// number of biquadratic patches
	const u32 biquadWidth = (controlWidth - 1)/2;
	const u32 biquadHeight = (controlHeight -1)/2;

	if ( LoadParam.verbose > 1 )
	{
		LoadParam.startTime = os::Timer::getRealTime();
	}

	// Create space for a temporary array of the patch's control points
	core::array<S3DVertex2TCoords_64> controlPoint;
	controlPoint.set_used( controlWidth * controlHeight );

	for( j = 0; j < controlPoint.size(); ++j)
	{
		copy( &controlPoint[j], &Vertices [ face->vertexIndex + j ], storevertexcolor );
	}

	// create a temporary patch
	Bezier.Patch = new scene::SMeshBufferLightMap();

	//Loop through the biquadratic patches
	for( j = 0; j < biquadHeight; ++j)
	{
		for( k = 0; k < biquadWidth; ++k)
		{
			// set up this patch
			const s32 inx = j*controlWidth*2 + k*2;

			// setup bezier control points for this patch
			Bezier.control[0] = controlPoint[ inx + 0];
			Bezier.control[1] = controlPoint[ inx + 1];
			Bezier.control[2] = controlPoint[ inx + 2];
			Bezier.control[3] = controlPoint[ inx + controlWidth + 0 ];
			Bezier.control[4] = controlPoint[ inx + controlWidth + 1 ];
			Bezier.control[5] = controlPoint[ inx + controlWidth + 2 ];
			Bezier.control[6] = controlPoint[ inx + controlWidth * 2 + 0];
			Bezier.control[7] = controlPoint[ inx + controlWidth * 2 + 1];
			Bezier.control[8] = controlPoint[ inx + controlWidth * 2 + 2];

			Bezier.tesselate( patchTesselation );
		}
	}

	// stitch together with existing geometry
	// TODO: only border needs to be checked
	const u32 bsize = Bezier.Patch->getVertexCount();
	const u32 msize = meshBuffer->getVertexCount();
/*
	for ( j = 0; j!= bsize; ++j )
	{
		const core::vector3df &v = Bezier.Patch->Vertices[j].Pos;

		for ( k = 0; k!= msize; ++k )
		{
			const core::vector3df &m = meshBuffer->Vertices[k].Pos;

			if ( !v.equals( m, tolerance ) )
				continue;

			meshBuffer->Vertices[k].Pos = v;
			//Bezier.Patch->Vertices[j].Pos = m;
		}
	}
*/

	// add Patch to meshbuffer
	meshBuffer->Vertices.reallocate(msize+bsize);
	for ( j = 0; j!= bsize; ++j )
	{
		meshBuffer->Vertices.push_back( Bezier.Patch->Vertices[j] );
	}

	// add indices to meshbuffer
	meshBuffer->Indices.reallocate(meshBuffer->getIndexCount()+Bezier.Patch->getIndexCount());
	for ( j = 0; j!= Bezier.Patch->getIndexCount(); ++j )
	{
		meshBuffer->Indices.push_back( msize + Bezier.Patch->Indices[j] );
	}

	delete Bezier.Patch;

	if ( LoadParam.verbose > 1 )
	{
		LoadParam.endTime = os::Timer::getRealTime();

		snprintf( buf, sizeof ( buf ),
			"quake3::createCurvedSurface_bezier needed %04d ms to create bezier patch.(%dx%d)",
			LoadParam.endTime - LoadParam.startTime,
			biquadWidth,
			biquadHeight
			);
		os::Printer::log(buf, ELL_INFORMATION);
	}

}



/*!
	Loads entities from file
*/
void CQ3LevelMesh::getConfiguration( io::IReadFile* file )
{
	tBSPLump l;
	l.offset = file->getPos();
	l.length = file->getSize ();

	core::array<u8> entity;
	entity.set_used( l.length + 2 );
	entity[l.length + 1 ] = 0;

	file->seek(l.offset);
	file->read( entity.pointer(), l.length);

	parser_parse( entity.pointer(), l.length, &CQ3LevelMesh::scriptcallback_config );

	if ( Entity.size () )
		Entity.getLast().name = file->getFileName();
}


//! get's an interface to the entities
tQ3EntityList & CQ3LevelMesh::getEntityList()
{
//	Entity.sort();
	return Entity;
}

//! returns the requested brush entity
IMesh* CQ3LevelMesh::getBrushEntityMesh(s32 num) const
{
	if (num < 1 || num >= NumModels)
		return 0;

	return BrushEntities[num];
}

//! returns the requested brush entity
IMesh* CQ3LevelMesh::getBrushEntityMesh(quake3::IEntity &ent) const
{
	// This is a helper function to parse the entity,
	// so you don't have to.

	s32 num;

	const quake3::SVarGroup* group = ent.getGroup(1);
	const core::stringc& modnum = group->get("model");

	if (!group->isDefined("model"))
		return 0;

	const char *temp = modnum.c_str() + 1; // We skip the first character.
	num = core::strtol10(temp);

	return getBrushEntityMesh(num);
}


/*!
*/
const IShader * CQ3LevelMesh::getShader(u32 index) const
{
	index &= 0xFFFF;

	if ( index < Shader.size() )
	{
		return &Shader[index];
	}

	return 0;
}


/*!
	loads the shader definition
*/
const IShader* CQ3LevelMesh::getShader( const c8 * filename, bool fileNameIsValid )
{
	core::stringc searchName ( filename );

	IShader search;
	search.name = searchName;
	search.name.replace( '\\', '/' );
	search.name.make_lower();


	core::stringc message;
	s32 index;

	//! is Shader already in cache?
	index = Shader.linear_search( search );
	if ( index >= 0 )
	{
		if ( LoadParam.verbose > 1 )
		{
			message = searchName + " found " + Shader[index].name;
			os::Printer::log("quake3:getShader", message.c_str(), ELL_INFORMATION);
		}

		return &Shader[index];
	}

	io::path loadFile;

	if ( !fileNameIsValid )
	{
		// extract the shader name from the last path component in filename
		// "scripts/[name].shader"
		core::stringc cut( search.name );

		s32 end = cut.findLast( '/' );
		s32 start = cut.findLast( '/', end - 1 );

		loadFile = LoadParam.scriptDir;
		loadFile.append( cut.subString( start, end - start ) );
		loadFile.append( ".shader" );
	}
	else
	{
		loadFile = search.name;
	}

	// already loaded the file ?
	index = ShaderFile.binary_search( loadFile );
	if ( index >= 0 )
		return 0;

	// add file to loaded files
	ShaderFile.push_back( loadFile );

	if ( !FileSystem->existFile( loadFile.c_str() ) )
	{
		if ( LoadParam.verbose > 1 )
		{
			message = loadFile + " for " + searchName + " failed ";
			os::Printer::log("quake3:getShader", message.c_str(), ELL_INFORMATION);
		}
		return 0;
	}

	if ( LoadParam.verbose )
	{
		message = loadFile + " for " + searchName;
		os::Printer::log("quake3:getShader Load shader", message.c_str(), ELL_INFORMATION);
	}


	io::IReadFile *file = FileSystem->createAndOpenFile( loadFile.c_str() );
	if ( file )
	{
		getShader ( file );
		file->drop ();
	}


	// search again
	index = Shader.linear_search( search );
	return index >= 0 ? &Shader[index] : 0;
}

/*!
	loads the shader definition
*/
void CQ3LevelMesh::getShader( io::IReadFile* file )
{
	if ( 0 == file )
		return;

	// load script
	core::array<u8> script;
	const long len = file->getSize();

	script.set_used( len + 2 );

	file->seek( 0 );
	file->read( script.pointer(), len );
	script[ len + 1 ] = 0;

	// start a parser instance
	parser_parse( script.pointer(), len, &CQ3LevelMesh::scriptcallback_shader );
}


//! adding default shaders
void CQ3LevelMesh::InitShader()
{
	ReleaseShader();

	IShader element;

	SVarGroup group;
	SVariable variable ( "noshader" );

	group.Variable.push_back( variable );

	element.VarGroup = new SVarGroupList();
	element.VarGroup->VariableGroup.push_back( group );
	element.VarGroup->VariableGroup.push_back( SVarGroup() );
	element.name = element.VarGroup->VariableGroup[0].Variable[0].name;
	element.ID = Shader.size();
	Shader.push_back( element );

	if ( LoadParam.loadAllShaders )
	{
		io::EFileSystemType current = FileSystem->setFileListSystem ( io::FILESYSTEM_VIRTUAL );
		io::path save = FileSystem->getWorkingDirectory();

		io::path newDir;
		newDir = "/";
		newDir += LoadParam.scriptDir;
		newDir += "/";
		FileSystem->changeWorkingDirectoryTo ( newDir.c_str() );

		core::stringc s;
		io::IFileList *fileList = FileSystem->createFileList ();
		for (u32 i=0; i< fileList->getFileCount(); ++i)
		{
			s = fileList->getFullFileName(i);
			if ( s.find ( ".shader" ) >= 0 )
			{
				if ( 0 == LoadParam.loadSkyShader && s.find ( "sky.shader" ) >= 0 )
				{
				}
				else
				{
					getShader ( s.c_str () );
				}
			}
		}
		fileList->drop ();

		FileSystem->changeWorkingDirectoryTo ( save );
		FileSystem->setFileListSystem ( current );
	}
}


//! script callback for shaders
//! i'm having troubles with the reference counting, during callback.. resorting..
void CQ3LevelMesh::ReleaseShader()
{
	for ( u32 i = 0; i!= Shader.size(); ++i )
	{
		Shader[i].VarGroup->drop();
	}
	Shader.clear();
	ShaderFile.clear();
}


/*!
*/
void CQ3LevelMesh::ReleaseEntity()
{
	for ( u32 i = 0; i!= Entity.size(); ++i )
	{
		Entity[i].VarGroup->drop();
	}
	Entity.clear();
}


// config in simple (quake3) and advanced style
void CQ3LevelMesh::scriptcallback_config( SVarGroupList *& grouplist, eToken token )
{
	IShader element;

	if ( token == Q3_TOKEN_END_LIST )
	{
		if ( 0 == grouplist->VariableGroup[0].Variable.size() )
			return;

		element.name = grouplist->VariableGroup[0].Variable[0].name;
	}
	else
	{
		if ( grouplist->VariableGroup.size() != 2 )
			return;

		element.name = "configuration";
	}

	grouplist->grab();
	element.VarGroup = grouplist;
	element.ID = Entity.size();
	Entity.push_back( element );
}


// entity only has only one valid level.. and no assoziative name..
void CQ3LevelMesh::scriptcallback_entity( SVarGroupList *& grouplist, eToken token )
{
	if ( token != Q3_TOKEN_END_LIST || grouplist->VariableGroup.size() != 2 )
		return;

	grouplist->grab();

	IEntity element;
	element.VarGroup = grouplist;
	element.ID = Entity.size();
	element.name = grouplist->VariableGroup[1].get( "classname" );


	Entity.push_back( element );
}


//!. script callback for shaders
void CQ3LevelMesh::scriptcallback_shader( SVarGroupList *& grouplist,eToken token )
{
	if ( token != Q3_TOKEN_END_LIST || grouplist->VariableGroup[0].Variable.size()==0)
		return;


	IShader element;

	grouplist->grab();
	element.VarGroup = grouplist;
	element.name = element.VarGroup->VariableGroup[0].Variable[0].name;
	element.ID = Shader.size();
/*
	core::stringc s;
	dumpShader ( s, &element );
	printf ( s.c_str () );
*/
	Shader.push_back( element );
}


/*!
	delete all buffers without geometry in it.
*/
void CQ3LevelMesh::cleanMeshes()
{
	if ( 0 == LoadParam.cleanUnResolvedMeshes )
		return;

	s32 i;

	// First the main level
	for (i = 0; i < E_Q3_MESH_SIZE; i++)
	{
		bool texture0important = ( i == 0 );

		cleanMesh(Mesh[i], texture0important);
	}

	// Then the brush entities
	for (i = 1; i < NumModels; i++)
	{
		cleanMesh(BrushEntities[i], true);
	}
}

void CQ3LevelMesh::cleanMesh(SMesh *m, const bool texture0important)
{
	// delete all buffers without geometry in it.
	u32 run = 0;
	u32 remove = 0;

	IMeshBuffer *b;

	run = 0;
	remove = 0;

	if ( LoadParam.verbose > 0 )
	{
		LoadParam.startTime = os::Timer::getRealTime();
		if ( LoadParam.verbose > 1 )
		{
			snprintf( buf, sizeof ( buf ),
				"quake3::cleanMeshes start for %d meshes",
				m->MeshBuffers.size()
				);
			os::Printer::log(buf, ELL_INFORMATION);
		}
	}

	u32 i = 0;
	s32 blockstart = -1;
	s32 blockcount = 0;

	while( i < m->MeshBuffers.size())
	{
		run += 1;

		b = m->MeshBuffers[i];

		if ( b->getVertexCount() == 0 || b->getIndexCount() == 0 ||
			( texture0important && b->getMaterial().getTexture(0) == 0 )
			)
		{
			if ( blockstart < 0 )
			{
				blockstart = i;
				blockcount = 0;
			}
			blockcount += 1;
			i += 1;

			// delete Meshbuffer
			i -= 1;
			remove += 1;
			b->drop();
			m->MeshBuffers.erase(i);
		}
		else
		{
			// clean blockwise
			if ( blockstart >= 0 )
			{
				if ( LoadParam.verbose > 1 )
				{
					snprintf( buf, sizeof ( buf ),
						"quake3::cleanMeshes cleaning mesh %d %d size",
						blockstart,
						blockcount
						);
					os::Printer::log(buf, ELL_INFORMATION);
				}
				blockstart = -1;
			}
			i += 1;
		}
	}

	if ( LoadParam.verbose > 0 )
	{
		LoadParam.endTime = os::Timer::getRealTime();
		snprintf( buf, sizeof ( buf ),
			"quake3::cleanMeshes needed %04d ms to clean %d of %d meshes",
			LoadParam.endTime - LoadParam.startTime,
			remove,
			run
			);
		os::Printer::log(buf, ELL_INFORMATION);
	}
}


// recalculate bounding boxes
void CQ3LevelMesh::calcBoundingBoxes()
{
	if ( LoadParam.verbose > 0 )
	{
		LoadParam.startTime = os::Timer::getRealTime();

		if ( LoadParam.verbose > 1 )
		{
			snprintf( buf, sizeof ( buf ),
				"quake3::calcBoundingBoxes start create %d textures and %d lightmaps",
				NumTextures,
				NumLightMaps
				);
			os::Printer::log(buf, ELL_INFORMATION);
		}
	}

	s32 g;

	// create bounding box
	for ( g = 0; g != E_Q3_MESH_SIZE; ++g )
	{
		for ( u32 j=0; j < Mesh[g]->MeshBuffers.size(); ++j)
		{
			((SMeshBufferLightMap*)Mesh[g]->MeshBuffers[j])->recalculateBoundingBox();
		}

		Mesh[g]->recalculateBoundingBox();
		// Mesh[0] is the main bbox
		if (g!=0)
			Mesh[0]->BoundingBox.addInternalBox(Mesh[g]->getBoundingBox());
	}

	for (g = 1; g < NumModels; g++)
	{
		for ( u32 j=0; j < BrushEntities[g]->MeshBuffers.size(); ++j)
		{
			((SMeshBufferLightMap*)BrushEntities[g]->MeshBuffers[j])->
				recalculateBoundingBox();
		}

		BrushEntities[g]->recalculateBoundingBox();
	}

	if ( LoadParam.verbose > 0 )
	{
		LoadParam.endTime = os::Timer::getRealTime();

		snprintf( buf, sizeof ( buf ),
			"quake3::calcBoundingBoxes needed %04d ms to create %d textures and %d lightmaps",
			LoadParam.endTime - LoadParam.startTime,
			NumTextures,
			NumLightMaps
			);
		os::Printer::log( buf, ELL_INFORMATION);
	}
}


//! loads the textures
void CQ3LevelMesh::loadTextures()
{
	if (!Driver)
		return;

	if ( LoadParam.verbose > 0 )
	{
		LoadParam.startTime = os::Timer::getRealTime();

		if ( LoadParam.verbose > 1 )
		{
			snprintf( buf, sizeof ( buf ),
				"quake3::loadTextures start create %d textures and %d lightmaps",
				NumTextures,
				NumLightMaps
				);
			os::Printer::log( buf, ELL_INFORMATION);
		}
	}

	c8 lightmapname[255];
	s32 t;

	// load lightmaps.
	Lightmap.set_used(NumLightMaps);

/*
	bool oldMipMapState = Driver->getTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS);
	Driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, false);
*/
	core::dimension2d<u32> lmapsize(128,128);

	video::IImage* lmapImg;
	for ( t = 0; t < NumLightMaps ; ++t)
	{
		sprintf(lightmapname, "%s.lightmap.%d", LevelName.c_str(), t);

		// lightmap is a CTexture::R8G8B8 format
		lmapImg = Driver->createImageFromData(
			video::ECF_R8G8B8, lmapsize,
			LightMaps[t].imageBits, false, true );

		Lightmap[t] = Driver->addTexture( lightmapname, lmapImg );
		lmapImg->drop();
	}

//	Driver->setTextureCreationFlag(video::ETCF_CREATE_MIP_MAPS, oldMipMapState);

	// load textures
	Tex.set_used( NumTextures );

	const IShader* shader;

	core::stringc list;
	io::path check;
	tTexArray textureArray;

	// pre-load shaders
	for ( t=0; t< NumTextures; ++t)
	{
		shader = getShader(Textures[t].strName, false);
	}

	for ( t=0; t< NumTextures; ++t)
	{
		Tex[t].ShaderID = -1;
		Tex[t].Texture = 0;

		list = "";

		// get a shader ( if one exists )
		shader = getShader( Textures[t].strName, false);
		if ( shader )
		{
			Tex[t].ShaderID = shader->ID;

			// if texture name == stage1 Texture map
			const SVarGroup * group;

			group = shader->getGroup( 2 );
			if ( group )
			{
				if ( core::cutFilenameExtension( check, group->get( "map" ) ) == Textures[t].strName )
				{
					list += check;
				}
				else
				if ( check == "$lightmap" )
				{
					// we check if lightmap is in stage 1 and texture in stage 2
					group = shader->getGroup( 3 );
					if ( group )
						list += group->get( "map" );
				}
			}
		}
		else
		{
			// no shader, take it
			list += Textures[t].strName;
		}

		u32 pos = 0;
		getTextures( textureArray, list, pos, FileSystem, Driver );

		Tex[t].Texture = textureArray[0];
	}

	if ( LoadParam.verbose > 0 )
	{
		LoadParam.endTime = os::Timer::getRealTime();

		snprintf( buf, sizeof ( buf ),
			"quake3::loadTextures needed %04d ms to create %d textures and %d lightmaps",
			LoadParam.endTime - LoadParam.startTime,
			NumTextures,
			NumLightMaps
			);
		os::Printer::log( buf, ELL_INFORMATION);
	}
}


//! Returns an axis aligned bounding box of the mesh.
const core::aabbox3d<f32>& CQ3LevelMesh::getBoundingBox() const
{
	return Mesh[0]->getBoundingBox();
}


void CQ3LevelMesh::setBoundingBox(const core::aabbox3df& box)
{
	Mesh[0]->setBoundingBox(box);
}


//! Returns the type of the animated mesh.
E_ANIMATED_MESH_TYPE CQ3LevelMesh::getMeshType() const
{
	return scene::EAMT_BSP;
}

} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_BSP_LOADER_
