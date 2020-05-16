// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#ifdef _IRR_COMPILE_WITH_COLLADA_LOADER_

#include "CColladaFileLoader.h"
#include "os.h"
#include "IXMLReader.h"
#include "IDummyTransformationSceneNode.h"
#include "SAnimatedMesh.h"
#include "fast_atof.h"
#include "quaternion.h"
#include "ILightSceneNode.h"
#include "ICameraSceneNode.h"
#include "IMeshManipulator.h"
#include "IReadFile.h"
#include "IAttributes.h"
#include "IMeshCache.h"
#include "IMeshSceneNode.h"
#include "SMeshBufferLightMap.h"
#include "irrMap.h"

#ifdef _DEBUG
#define COLLADA_READER_DEBUG
#endif
namespace irr
{
namespace scene
{
namespace
{
	// currently supported COLLADA tag names
	const core::stringc colladaSectionName =   "COLLADA";
	const core::stringc librarySectionName =   "library";
	const core::stringc libraryNodesSectionName = "library_nodes";
	const core::stringc libraryGeometriesSectionName = "library_geometries";
	const core::stringc libraryMaterialsSectionName = "library_materials";
	const core::stringc libraryImagesSectionName = "library_images";
	const core::stringc libraryVisualScenesSectionName = "library_visual_scenes";
	const core::stringc libraryCamerasSectionName = "library_cameras";
	const core::stringc libraryLightsSectionName = "library_lights";
	const core::stringc libraryEffectsSectionName = "library_effects";
	const core::stringc assetSectionName =     "asset";
	const core::stringc sceneSectionName =     "scene";
	const core::stringc visualSceneSectionName = "visual_scene";

	const core::stringc lightPrefabName =      "light";
	const core::stringc cameraPrefabName =     "camera";
	const core::stringc materialSectionName =  "material";
	const core::stringc geometrySectionName =  "geometry";
	const core::stringc imageSectionName =     "image";
	const core::stringc textureSectionName =   "texture";
	const core::stringc effectSectionName =    "effect";

	const core::stringc pointSectionName =     "point";
	const core::stringc directionalSectionName ="directional";
	const core::stringc spotSectionName =      "spot";
	const core::stringc ambientSectionName =   "ambient";
	const core::stringc meshSectionName =      "mesh";
	const core::stringc sourceSectionName =    "source";
	const core::stringc arraySectionName =     "array";
	const core::stringc floatArraySectionName ="float_array";
	const core::stringc intArraySectionName =  "int_array";
	const core::stringc techniqueCommonSectionName = "technique_common";
	const core::stringc accessorSectionName =  "accessor";
	const core::stringc verticesSectionName =  "vertices";
	const core::stringc inputTagName =         "input";
	const core::stringc polylistSectionName =  "polylist";
	const core::stringc trianglesSectionName = "triangles";
	const core::stringc polygonsSectionName =  "polygons";
	const core::stringc primitivesName =       "p";
	const core::stringc vcountName =           "vcount";

	const core::stringc upAxisNodeName =       "up_axis";
	const core::stringc nodeSectionName =      "node";
	const core::stringc lookatNodeName =       "lookat";
	const core::stringc matrixNodeName =       "matrix";
	const core::stringc perspectiveNodeName =  "perspective";
	const core::stringc rotateNodeName =       "rotate";
	const core::stringc scaleNodeName =        "scale";
	const core::stringc translateNodeName =    "translate";
	const core::stringc skewNodeName =         "skew";
	const core::stringc bboxNodeName =         "boundingbox";
	const core::stringc minNodeName =          "min";
	const core::stringc maxNodeName =          "max";
	const core::stringc instanceName =         "instance";
	const core::stringc instanceGeometryName = "instance_geometry";
	const core::stringc instanceSceneName =    "instance_visual_scene";
	const core::stringc instanceEffectName =   "instance_effect";
	const core::stringc instanceMaterialName = "instance_material";
	const core::stringc instanceLightName =    "instance_light";
	const core::stringc instanceNodeName =     "instance_node";
	const core::stringc bindMaterialName =     "bind_material";
	const core::stringc extraNodeName =        "extra";
	const core::stringc techniqueNodeName =    "technique";
	const core::stringc colorNodeName =        "color";
	const core::stringc floatNodeName =        "float";
	const core::stringc float2NodeName =       "float2";
	const core::stringc float3NodeName =       "float3";

	const core::stringc newParamName =         "newparam";
	const core::stringc paramTagName =         "param";
	const core::stringc initFromName =         "init_from";
	const core::stringc dataName =             "data";
	const core::stringc wrapsName =            "wrap_s";
	const core::stringc wraptName =            "wrap_t";
	const core::stringc minfilterName =        "minfilter";
	const core::stringc magfilterName =        "magfilter";
	const core::stringc mipfilterName =        "mipfilter";

	const core::stringc textureNodeName =      "texture";
	const core::stringc doubleSidedNodeName =  "double_sided";
	const core::stringc constantAttenuationNodeName = "constant_attenuation";
	const core::stringc linearAttenuationNodeName = "linear_attenuation";
	const core::stringc quadraticAttenuationNodeName = "quadratic_attenuation";
	const core::stringc falloffAngleNodeName = "falloff_angle";
	const core::stringc falloffExponentNodeName = "falloff_exponent";

	const core::stringc profileCOMMONSectionName = "profile_COMMON";
	const core::stringc profileCOMMONAttributeName = "COMMON";

	const char* const inputSemanticNames[] = {"POSITION", "VERTEX", "NORMAL", "TEXCOORD",
		"UV", "TANGENT", "IMAGE", "TEXTURE", 0};

	// We have to read ambient lights like other light types here, so we need a type for it
	const video::E_LIGHT_TYPE ELT_AMBIENT = video::E_LIGHT_TYPE(video::ELT_COUNT+1);
}

	//! following class is for holding and creating instances of library
	//! objects, named prefabs in this loader.
	class CPrefab : public IColladaPrefab
	{
	public:

		CPrefab(const core::stringc& id) : Id(id)
		{
		}

		//! creates an instance of this prefab
		virtual scene::ISceneNode* addInstance(scene::ISceneNode* parent,
			scene::ISceneManager* mgr)
		{
			// empty implementation
			return 0;
		}

		//! returns id of this prefab
		virtual const core::stringc& getId()
		{
			return Id;
		}

	protected:

		core::stringc Id;
	};


	//! prefab for a light scene node
	class CLightPrefab : public CPrefab
	{
	public:

		CLightPrefab(const core::stringc& id) : CPrefab(id)
		{
			#ifdef COLLADA_READER_DEBUG
			os::Printer::log("COLLADA: loaded light prefab", Id.c_str(), ELL_DEBUG);
			#endif
		}

		video::SLight LightData; // publically accessible

		//! creates an instance of this prefab
		virtual scene::ISceneNode* addInstance(scene::ISceneNode* parent,
			scene::ISceneManager* mgr)
		{
			#ifdef COLLADA_READER_DEBUG
			os::Printer::log("COLLADA: Constructing light instance", Id.c_str(), ELL_DEBUG);
			#endif

			if ( LightData.Type == ELT_AMBIENT )
			{
				mgr->setAmbientLight( LightData.DiffuseColor );
				return 0;
			}

			scene::ILightSceneNode* l = mgr->addLightSceneNode(parent);
			if (l)
			{
				l->setLightData ( LightData );
				l->setName(getId());
			}
			return l;
		}
	};


	//! prefab for a mesh scene node
	class CGeometryPrefab : public CPrefab
	{
	public:

		CGeometryPrefab(const core::stringc& id) : CPrefab(id)
		{
		}

		scene::IMesh* Mesh;

		//! creates an instance of this prefab
		virtual scene::ISceneNode* addInstance(scene::ISceneNode* parent,
			scene::ISceneManager* mgr)
		{
			#ifdef COLLADA_READER_DEBUG
			os::Printer::log("COLLADA: Constructing mesh instance", Id.c_str(), ELL_DEBUG);
			#endif

			scene::ISceneNode* m = mgr->addMeshSceneNode(Mesh, parent);
			if (m)
			{
				m->setName(getId());
//				m->setMaterialFlag(video::EMF_BACK_FACE_CULLING, false);
//				m->setDebugDataVisible(scene::EDS_FULL);
			}
			return m;
		}
	};


	//! prefab for a camera scene node
	class CCameraPrefab : public CPrefab
	{
	public:

		CCameraPrefab(const core::stringc& id)
			: CPrefab(id), YFov(core::PI / 2.5f), ZNear(1.0f), ZFar(3000.0f)
		{
			#ifdef COLLADA_READER_DEBUG
			os::Printer::log("COLLADA: loaded camera prefab", Id.c_str(), ELL_DEBUG);
			#endif
		}

		// publicly accessible data
		f32 YFov;
		f32 ZNear;
		f32 ZFar;

		//! creates an instance of this prefab
		virtual scene::ISceneNode* addInstance(scene::ISceneNode* parent,
			scene::ISceneManager* mgr)
		{
			#ifdef COLLADA_READER_DEBUG
			os::Printer::log("COLLADA: Constructing camera instance", Id.c_str(), ELL_DEBUG);
			#endif

			scene::ICameraSceneNode* c = mgr->addCameraSceneNode(parent);
			if (c)
			{
				c->setFOV(YFov);
				c->setNearValue(ZNear);
				c->setFarValue(ZFar);
				c->setName(getId());
			}
			return c;
		}
	};


	//! prefab for a container scene node
	//! Collects other prefabs and instantiates them upon instantiation
	//! Uses a dummy scene node to return the children as one scene node
	class CScenePrefab : public CPrefab
	{
	public:
		CScenePrefab(const core::stringc& id) : CPrefab(id)
		{
			#ifdef COLLADA_READER_DEBUG
			os::Printer::log("COLLADA: loaded scene prefab", Id.c_str(), ELL_DEBUG);
			#endif
		}

		//! creates an instance of this prefab
		virtual scene::ISceneNode* addInstance(scene::ISceneNode* parent,
			scene::ISceneManager* mgr)
		{
			#ifdef COLLADA_READER_DEBUG
			os::Printer::log("COLLADA: Constructing scene instance", Id.c_str(), ELL_DEBUG);
			#endif

			if (Children.size()==0)
				return 0;

			scene::IDummyTransformationSceneNode* s = mgr->addDummyTransformationSceneNode(parent);
			if (s)
			{
				s->setName(getId());
				s->getRelativeTransformationMatrix() = Transformation;
				s->updateAbsolutePosition();
				core::stringc t;
				for (u32 i=0; i<16; ++i)
				{
					t+=core::stringc((double)Transformation[i]);
					t+=" ";
				}
			#ifdef COLLADA_READER_DEBUG
			os::Printer::log("COLLADA: Transformation", t.c_str(), ELL_DEBUG);
			#endif

				for (u32 i=0; i<Children.size(); ++i)
					Children[i]->addInstance(s, mgr);
			}

			return s;
		}

		core::array<IColladaPrefab*> Children;
		core::matrix4 Transformation;
	};


//! Constructor
CColladaFileLoader::CColladaFileLoader(scene::ISceneManager* smgr,
		io::IFileSystem* fs)
: SceneManager(smgr), FileSystem(fs), DummyMesh(0),
	FirstLoadedMesh(0), LoadedMeshCount(0), CreateInstances(false)
{
	#ifdef _DEBUG
	setDebugName("CColladaFileLoader");
	#endif
}


//! destructor
CColladaFileLoader::~CColladaFileLoader()
{
	if (DummyMesh)
		DummyMesh->drop();

	if (FirstLoadedMesh)
		FirstLoadedMesh->drop();
}


//! Returns true if the file maybe is able to be loaded by this class.
/** This decision should be based only on the file extension (e.g. ".cob") */
bool CColladaFileLoader::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension ( filename, "xml", "dae" );
}


//! creates/loads an animated mesh from the file.
//! \return Pointer to the created mesh. Returns 0 if loading failed.
//! If you no longer need the mesh, you should call IAnimatedMesh::drop().
//! See IReferenceCounted::drop() for more information.
IAnimatedMesh* CColladaFileLoader::createMesh(io::IReadFile* file)
{
	io::IXMLReaderUTF8* reader = FileSystem->createXMLReaderUTF8(file);
	if (!reader)
		return 0;

	CurrentlyLoadingMesh = file->getFileName();
	CreateInstances = SceneManager->getParameters()->getAttributeAsBool(
		scene::COLLADA_CREATE_SCENE_INSTANCES);
	Version = 0;
	FlipAxis = false;

	// read until COLLADA section, skip other parts

	while(reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT)
		{
			if (colladaSectionName == reader->getNodeName())
				readColladaSection(reader);
			else
				skipSection(reader, true); // unknown section
		}
	}

	reader->drop();
	if (!Version)
		return 0;

	// because this loader loads and creates a complete scene instead of
	// a single mesh, return an empty dummy mesh to make the scene manager
	// know that everything went well.
	if (!DummyMesh)
		DummyMesh = new SAnimatedMesh();
	scene::IAnimatedMesh* returnMesh = DummyMesh;

	if (Version < 10400)
		instantiateNode(SceneManager->getRootSceneNode());

	// add the first loaded mesh into the mesh cache too, if more than one
	// meshes have been loaded from the file
	if (LoadedMeshCount>1 && FirstLoadedMesh)
	{
		os::Printer::log("Added COLLADA mesh", FirstLoadedMeshName.c_str());
		SceneManager->getMeshCache()->addMesh(FirstLoadedMeshName.c_str(), FirstLoadedMesh);
	}

	// clean up temporary loaded data
	clearData();

	returnMesh->grab(); // store until this loader is destroyed

	DummyMesh->drop();
	DummyMesh = 0;

	if (FirstLoadedMesh)
		FirstLoadedMesh->drop();
	FirstLoadedMesh = 0;
	LoadedMeshCount = 0;

	return returnMesh;
}


//! skips an (unknown) section in the collada document
void CColladaFileLoader::skipSection(io::IXMLReaderUTF8* reader, bool reportSkipping)
{
	#ifndef COLLADA_READER_DEBUG
	if (reportSkipping) // always report in COLLADA_READER_DEBUG mode
	#endif
		os::Printer::log("COLLADA skipping section", core::stringc(reader->getNodeName()).c_str(), ELL_DEBUG);

	// skip if this element is empty anyway.
	if (reader->isEmptyElement())
		return;

	// read until we've reached the last element in this section
	u32 tagCounter = 1;

	while(tagCounter && reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT &&
			!reader->isEmptyElement())
		{
			#ifdef COLLADA_READER_DEBUG
			if (reportSkipping)
				os::Printer::log("Skipping COLLADA unknown element", core::stringc(reader->getNodeName()).c_str(), ELL_DEBUG);
			#endif

			++tagCounter;
		}
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
			--tagCounter;
	}
}


//! reads the <COLLADA> section and its content
void CColladaFileLoader::readColladaSection(io::IXMLReaderUTF8* reader)
{
	if (reader->isEmptyElement())
		return;

	// todo: patch level needs to be handled
	const f32 version = core::fast_atof(core::stringc(reader->getAttributeValue("version")).c_str());
	Version = core::floor32(version)*10000+core::round32(core::fract(version)*1000.0f);
	// Version 1.4 can be checked for by if (Version >= 10400)

	while(reader->read())
	if (reader->getNodeType() == io::EXN_ELEMENT)
	{
		if (assetSectionName == reader->getNodeName())
			readAssetSection(reader);
		else
		if (librarySectionName == reader->getNodeName())
			readLibrarySection(reader);
		else
		if (libraryNodesSectionName == reader->getNodeName())
			readLibrarySection(reader);
		else
		if (libraryGeometriesSectionName == reader->getNodeName())
			readLibrarySection(reader);
		else
		if (libraryMaterialsSectionName == reader->getNodeName())
			readLibrarySection(reader);
		else
		if (libraryEffectsSectionName == reader->getNodeName())
			readLibrarySection(reader);
		else
		if (libraryImagesSectionName == reader->getNodeName())
			readLibrarySection(reader);
		else
		if (libraryCamerasSectionName == reader->getNodeName())
			readLibrarySection(reader);
		else
		if (libraryLightsSectionName == reader->getNodeName())
			readLibrarySection(reader);
		else
		if (libraryVisualScenesSectionName == reader->getNodeName())
			readVisualScene(reader);
		else
		if (assetSectionName == reader->getNodeName())
			readAssetSection(reader);
		else
		if (sceneSectionName == reader->getNodeName())
			readSceneSection(reader);
		else
		{
			os::Printer::log("COLLADA loader warning: Wrong tag usage found", reader->getNodeName(), ELL_WARNING);
			skipSection(reader, true); // unknown section
		}
	}
}


//! reads a <library> section and its content
void CColladaFileLoader::readLibrarySection(io::IXMLReaderUTF8* reader)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading library", ELL_DEBUG);
	#endif

	if (reader->isEmptyElement())
		return;

	while(reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT)
		{
			// animation section tbd
			if (cameraPrefabName == reader->getNodeName())
				readCameraPrefab(reader);
			else
			// code section tbd
			// controller section tbd
			if (geometrySectionName == reader->getNodeName())
				readGeometry(reader);
			else
			if (imageSectionName == reader->getNodeName())
				readImage(reader);
			else
			if (lightPrefabName == reader->getNodeName())
				readLightPrefab(reader);
			else
			if (materialSectionName == reader->getNodeName())
				readMaterial(reader);
			else
			if (nodeSectionName == reader->getNodeName())
			{
				CScenePrefab p("");

				readNodeSection(reader, SceneManager->getRootSceneNode(), &p);
			}
			else
			if (effectSectionName == reader->getNodeName())
				readEffect(reader);
			else
			// program section tbd
			if (textureSectionName == reader->getNodeName())
				readTexture(reader);
			else
				skipSection(reader, true); // unknown section, not all allowed supported yet
		}
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
		{
			if (librarySectionName == reader->getNodeName())
				break; // end reading.
			if (libraryNodesSectionName == reader->getNodeName())
				break; // end reading.
			if (libraryGeometriesSectionName == reader->getNodeName())
				break; // end reading.
			if (libraryMaterialsSectionName == reader->getNodeName())
				break; // end reading.
			if (libraryEffectsSectionName == reader->getNodeName())
				break; // end reading.
			if (libraryImagesSectionName == reader->getNodeName())
				break; // end reading.
			if (libraryLightsSectionName == reader->getNodeName())
				break; // end reading.
			if (libraryCamerasSectionName == reader->getNodeName())
				break; // end reading.
		}
	}
}


//! reads a <visual_scene> element and stores it as a prefab
void CColladaFileLoader::readVisualScene(io::IXMLReaderUTF8* reader)
{
	CScenePrefab* p = 0;
	while(reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT)
		{
			if (visualSceneSectionName == reader->getNodeName())
				p = new CScenePrefab(readId(reader));
			else
			if (p && nodeSectionName == reader->getNodeName()) // as a child of visual_scene
				readNodeSection(reader, SceneManager->getRootSceneNode(), p);
			else
			if (assetSectionName == reader->getNodeName())
				readAssetSection(reader);
			else
			if (extraNodeName == reader->getNodeName())
				skipSection(reader, false); // ignore all other sections
			else
			{
				os::Printer::log("COLLADA loader warning: Wrong tag usage found", reader->getNodeName(), ELL_WARNING);
				skipSection(reader, true); // ignore all other sections
			}
		}
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
		{
			if (libraryVisualScenesSectionName == reader->getNodeName())
				return;
			else
			if ((visualSceneSectionName == reader->getNodeName()) && p)
			{
				Prefabs.push_back(p);
				p = 0;
			}
		}
	}
}


//! reads a <scene> section and its content
void CColladaFileLoader::readSceneSection(io::IXMLReaderUTF8* reader)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading scene", ELL_DEBUG);
	#endif

	if (reader->isEmptyElement())
		return;

	// read the scene

	core::matrix4 transform; // transformation of this node
	core::aabbox3df bbox;
	scene::IDummyTransformationSceneNode* node = 0;

	while(reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT)
		{
			if (lookatNodeName == reader->getNodeName())
				transform *= readLookAtNode(reader);
			else
			if (matrixNodeName == reader->getNodeName())
				transform *= readMatrixNode(reader);
			else
			if (perspectiveNodeName == reader->getNodeName())
				transform *= readPerspectiveNode(reader);
			else
			if (rotateNodeName == reader->getNodeName())
				transform *= readRotateNode(reader);
			else
			if (scaleNodeName == reader->getNodeName())
				transform *= readScaleNode(reader);
			else
			if (skewNodeName == reader->getNodeName())
				transform *= readSkewNode(reader);
			else
			if (translateNodeName == reader->getNodeName())
				transform *= readTranslateNode(reader);
			else
			if (bboxNodeName == reader->getNodeName())
				readBboxNode(reader, bbox);
			else
			if (nodeSectionName == reader->getNodeName())
			{
				// create dummy node if there is none yet.
				if (!node)
					node = SceneManager->addDummyTransformationSceneNode(SceneManager->getRootSceneNode());

				readNodeSection(reader, node);
			}
			else
			if ((instanceSceneName == reader->getNodeName()))
				readInstanceNode(reader, SceneManager->getRootSceneNode(), 0, 0,instanceSceneName);
			else
			if (extraNodeName == reader->getNodeName())
				skipSection(reader, false);
			else
			{
				os::Printer::log("COLLADA loader warning: Wrong tag usage found", reader->getNodeName(), ELL_WARNING);
				skipSection(reader, true); // ignore all other sections
			}
		}
		else
		if ((reader->getNodeType() == io::EXN_ELEMENT_END) &&
			(sceneSectionName == reader->getNodeName()))
				return;
	}
	if (node)
		node->getRelativeTransformationMatrix() = transform;
}


//! reads a <asset> section and its content
void CColladaFileLoader::readAssetSection(io::IXMLReaderUTF8* reader)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading asset", ELL_DEBUG);
	#endif

	if (reader->isEmptyElement())
		return;

	while(reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT)
		{
			if (upAxisNodeName == reader->getNodeName())
			{
					reader->read();
					FlipAxis = (core::stringc("Z_UP") == reader->getNodeData());
			}
		}
		else
		if ((reader->getNodeType() == io::EXN_ELEMENT_END) &&
			(assetSectionName == reader->getNodeName()))
				return;
	}
}


//! reads a <node> section and its content
void CColladaFileLoader::readNodeSection(io::IXMLReaderUTF8* reader, scene::ISceneNode* parent, CScenePrefab* p)
{
	if (reader->isEmptyElement())
	{
		return;
		#ifdef COLLADA_READER_DEBUG
		os::Printer::log("COLLADA reading empty node", ELL_DEBUG);
		#endif
	}

	core::stringc name = readId(reader);
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading node", name, ELL_DEBUG);
	#endif

	core::matrix4 transform; // transformation of this node
	core::aabbox3df bbox;
	scene::ISceneNode* node = 0; // instance
	CScenePrefab* nodeprefab = 0; // prefab for library_nodes usage

	if (p)
	{
		nodeprefab = new CScenePrefab(readId(reader));
		p->Children.push_back(nodeprefab);
		Prefabs.push_back(nodeprefab); // in order to delete them later on
	}

	// read the node

	while(reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT)
		{
			if (assetSectionName == reader->getNodeName())
				readAssetSection(reader);
			else
			if (lookatNodeName == reader->getNodeName())
				transform *= readLookAtNode(reader);
			else
			if (matrixNodeName == reader->getNodeName())
				transform *= readMatrixNode(reader);
			else
			if (perspectiveNodeName == reader->getNodeName())
				transform *= readPerspectiveNode(reader);
			else
			if (rotateNodeName == reader->getNodeName())
				transform *= readRotateNode(reader);
			else
			if (scaleNodeName == reader->getNodeName())
				transform *= readScaleNode(reader);
			else
			if (skewNodeName == reader->getNodeName())
				transform *= readSkewNode(reader);
			else
			if (translateNodeName == reader->getNodeName())
				transform *= readTranslateNode(reader);
			else
			if (bboxNodeName == reader->getNodeName())
				readBboxNode(reader, bbox);
			else
			if ((instanceName == reader->getNodeName()) ||
				(instanceNodeName == reader->getNodeName()) ||
				(instanceGeometryName == reader->getNodeName()) ||
				(instanceLightName == reader->getNodeName()))
			{
				scene::ISceneNode* newnode = 0;
				readInstanceNode(reader, parent, &newnode, nodeprefab, reader->getNodeName());

				if (node && newnode)
				{
					// move children from dummy to new node
					ISceneNodeList::ConstIterator it = node->getChildren().begin();
					for (; it != node->getChildren().end(); it = node->getChildren().begin())
						(*it)->setParent(newnode);

					// remove previous dummy node
					node->remove();
					node = newnode;
				}
			}
			else
			if (nodeSectionName == reader->getNodeName())
			{
				// create dummy node if there is none yet.
				if (CreateInstances && !node)
				{
					scene::IDummyTransformationSceneNode* dummy =
						SceneManager->addDummyTransformationSceneNode(parent);
					dummy->getRelativeTransformationMatrix() = transform;
					node = dummy;
				}
				else
					node = parent;

				// read and add child
				readNodeSection(reader, node, nodeprefab);
			}
			else
			if (extraNodeName == reader->getNodeName())
				skipSection(reader, false);
			else
				skipSection(reader, true); // ignore all other sections

		} // end if node
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
		{
			if (nodeSectionName == reader->getNodeName())
				break;
		}
	}

	if (nodeprefab)
		nodeprefab->Transformation = transform;
	else
	if (node)
	{
		// set transformation correctly into node.
		node->setPosition(transform.getTranslation());
		node->setRotation(transform.getRotationDegrees());
		node->setScale(transform.getScale());
		node->updateAbsolutePosition();

		node->setName(name);
	}
}


//! reads a <lookat> element and its content and creates a matrix from it
core::matrix4 CColladaFileLoader::readLookAtNode(io::IXMLReaderUTF8* reader)
{
	core::matrix4 mat;
	if (reader->isEmptyElement())
		return mat;

	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading look at node", ELL_DEBUG);
	#endif

	f32 floats[9];
	readFloatsInsideElement(reader, floats, 9);

	mat.buildCameraLookAtMatrixLH(
		core::vector3df(floats[0], floats[1], floats[2]),
		core::vector3df(floats[3], floats[4], floats[5]),
		core::vector3df(floats[6], floats[7], floats[8]));

	return mat;
}


//! reads a <skew> element and its content and creates a matrix from it
core::matrix4 CColladaFileLoader::readSkewNode(io::IXMLReaderUTF8* reader)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading skew node", ELL_DEBUG);
	#endif

	core::matrix4 mat;
	if (reader->isEmptyElement())
		return mat;

	f32 floats[7]; // angle rotation-axis translation-axis
	readFloatsInsideElement(reader, floats, 7);

	// build skew matrix from these 7 floats
	core::quaternion q;
	q.fromAngleAxis(floats[0]*core::DEGTORAD, core::vector3df(floats[1], floats[2], floats[3]));
	mat = q.getMatrix();

	if (floats[4]==1.f) // along x-axis
	{
		mat[4]=0.f;
		mat[6]=0.f;
		mat[8]=0.f;
		mat[9]=0.f;
	}
	else
	if (floats[5]==1.f) // along y-axis
	{
		mat[1]=0.f;
		mat[2]=0.f;
		mat[8]=0.f;
		mat[9]=0.f;
	}
	else
	if (floats[6]==1.f) // along z-axis
	{
		mat[1]=0.f;
		mat[2]=0.f;
		mat[4]=0.f;
		mat[6]=0.f;
	}

	return mat;
}


//! reads a <boundingbox> element and its content and stores it in bbox
void CColladaFileLoader::readBboxNode(io::IXMLReaderUTF8* reader,
		core::aabbox3df& bbox)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading boundingbox node", ELL_DEBUG);
	#endif

	bbox.reset(core::aabbox3df());

	if (reader->isEmptyElement())
		return;

	f32 floats[3];

	while(reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT)
		{
			if (minNodeName == reader->getNodeName())
			{
				readFloatsInsideElement(reader, floats, 3);
				bbox.MinEdge.set(floats[0], floats[1], floats[2]);
			}
			else
			if (maxNodeName == reader->getNodeName())
			{
				readFloatsInsideElement(reader, floats, 3);
				bbox.MaxEdge.set(floats[0], floats[1], floats[2]);
			}
			else
				skipSection(reader, true); // ignore all other sections
		}
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
		{
			if (bboxNodeName == reader->getNodeName())
				break;
		}
	}
}


//! reads a <matrix> element and its content and creates a matrix from it
core::matrix4 CColladaFileLoader::readMatrixNode(io::IXMLReaderUTF8* reader)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading matrix node", ELL_DEBUG);
	#endif

	core::matrix4 mat;
	if (reader->isEmptyElement())
		return mat;

	readFloatsInsideElement(reader, mat.pointer(), 16);
	// put translation into the correct place
	if (FlipAxis)
	{
		core::matrix4 mat2(mat, core::matrix4::EM4CONST_TRANSPOSED);
		mat2[1]=mat[8];
		mat2[2]=mat[4];
		mat2[4]=mat[2];
		mat2[5]=mat[10];
		mat2[6]=mat[6];
		mat2[8]=mat[1];
		mat2[9]=mat[9];
		mat2[10]=mat[5];
		mat2[12]=mat[3];
		mat2[13]=mat[11];
		mat2[14]=mat[7];
		return mat2;
	}
	else
		return core::matrix4(mat, core::matrix4::EM4CONST_TRANSPOSED);
}


//! reads a <perspective> element and its content and creates a matrix from it
core::matrix4 CColladaFileLoader::readPerspectiveNode(io::IXMLReaderUTF8* reader)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading perspective node", ELL_DEBUG);
	#endif

	core::matrix4 mat;
	if (reader->isEmptyElement())
		return mat;

	f32 floats[1];
	readFloatsInsideElement(reader, floats, 1);

	// TODO: build perspecitve matrix from this float

	os::Printer::log("COLLADA loader warning: <perspective> not implemented yet.", ELL_WARNING);

	return mat;
}


//! reads a <rotate> element and its content and creates a matrix from it
core::matrix4 CColladaFileLoader::readRotateNode(io::IXMLReaderUTF8* reader)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading rotate node", ELL_DEBUG);
	#endif

	core::matrix4 mat;
	if (reader->isEmptyElement())
		return mat;

	f32 floats[4];
	readFloatsInsideElement(reader, floats, 4);

	if (!core::iszero(floats[3]))
	{
		core::quaternion q;
		if (FlipAxis)
			q.fromAngleAxis(floats[3]*core::DEGTORAD, core::vector3df(floats[0], floats[2], floats[1]));
		else
			q.fromAngleAxis(floats[3]*core::DEGTORAD, core::vector3df(floats[0], floats[1], floats[2]));
		return q.getMatrix();
	}
	else
		return core::IdentityMatrix;
}


//! reads a <scale> element and its content and creates a matrix from it
core::matrix4 CColladaFileLoader::readScaleNode(io::IXMLReaderUTF8* reader)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading scale node", ELL_DEBUG);
	#endif

	core::matrix4 mat;
	if (reader->isEmptyElement())
		return mat;

	f32 floats[3];
	readFloatsInsideElement(reader, floats, 3);

	if (FlipAxis)
		mat.setScale(core::vector3df(floats[0], floats[2], floats[1]));
	else
		mat.setScale(core::vector3df(floats[0], floats[1], floats[2]));

	return mat;
}


//! reads a <translate> element and its content and creates a matrix from it
core::matrix4 CColladaFileLoader::readTranslateNode(io::IXMLReaderUTF8* reader)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading translate node", ELL_DEBUG);
	#endif

	core::matrix4 mat;
	if (reader->isEmptyElement())
		return mat;

	f32 floats[3];
	readFloatsInsideElement(reader, floats, 3);

	if (FlipAxis)
		mat.setTranslation(core::vector3df(floats[0], floats[2], floats[1]));
	else
		mat.setTranslation(core::vector3df(floats[0], floats[1], floats[2]));

	return mat;
}


//! reads any kind of <instance*> node
void CColladaFileLoader::readInstanceNode(io::IXMLReaderUTF8* reader,
		scene::ISceneNode* parent, scene::ISceneNode** outNode,
		CScenePrefab* p, const core::stringc& type)
{
	// find prefab of the specified id
	core::stringc url = reader->getAttributeValue("url");
	uriToId(url);

	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading instance", url, ELL_DEBUG);
	#endif

	if (!reader->isEmptyElement())
	{
		while(reader->read())
		{
			if (reader->getNodeType() == io::EXN_ELEMENT)
			{
				if (bindMaterialName == reader->getNodeName())
					readBindMaterialSection(reader,url);
				else
				if (extraNodeName == reader->getNodeName())
					skipSection(reader, false);
			}
			else
			if (reader->getNodeType() == io::EXN_ELEMENT_END)
				break;
		}
	}
	instantiateNode(parent, outNode, p, url, type);
}


void CColladaFileLoader::instantiateNode(scene::ISceneNode* parent,
		scene::ISceneNode** outNode, CScenePrefab* p, const core::stringc& url,
		const core::stringc& type)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA instantiate node", ELL_DEBUG);
	#endif

	for (u32 i=0; i<Prefabs.size(); ++i)
	{
		if (url == "" || url == Prefabs[i]->getId())
		{
			if (p)
				p->Children.push_back(Prefabs[i]);
			else
			if (CreateInstances)
			{
				scene::ISceneNode * newNode
					= Prefabs[i]->addInstance(parent, SceneManager);
				if (outNode)
				{
					*outNode = newNode;
					if (*outNode)
						(*outNode)->setName(url);
				}
			}
			return;
		}
	}
	if (p)
	{
		if (instanceGeometryName==type)
		{
			Prefabs.push_back(new CGeometryPrefab(url));
			p->Children.push_back(Prefabs.getLast());
		}
	}
}


//! reads a <camera> element and stores it as prefab
void CColladaFileLoader::readCameraPrefab(io::IXMLReaderUTF8* reader)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading camera prefab", ELL_DEBUG);
	#endif

	CCameraPrefab* prefab = new CCameraPrefab(readId(reader));

	if (!reader->isEmptyElement())
	{
		// read techniques optics and imager (the latter is completely ignored, though)
		readColladaParameters(reader, cameraPrefabName);

		SColladaParam* p;

		// XFOV not yet supported
		p = getColladaParameter(ECPN_YFOV);
		if (p && p->Type == ECPT_FLOAT)
			prefab->YFov = p->Floats[0];

		p = getColladaParameter(ECPN_ZNEAR);
		if (p && p->Type == ECPT_FLOAT)
			prefab->ZNear = p->Floats[0];

		p = getColladaParameter(ECPN_ZFAR);
		if (p && p->Type == ECPT_FLOAT)
			prefab->ZFar = p->Floats[0];
		// orthographic camera uses LEFT, RIGHT, TOP, and BOTTOM
	}

	Prefabs.push_back(prefab);
}


//! reads a <image> element and stores it in the image section
void CColladaFileLoader::readImage(io::IXMLReaderUTF8* reader)
{
	// add image to list of loaded images.
	Images.push_back(SColladaImage());
	SColladaImage& image=Images.getLast();

	image.Id = readId(reader);
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading image", core::stringc(image.Id), ELL_DEBUG);
	#endif
	image.Dimension.Height = (u32)reader->getAttributeValueAsInt("height");
	image.Dimension.Width = (u32)reader->getAttributeValueAsInt("width");

	if (Version >= 10400) // start with 1.4
	{
		while(reader->read())
		{
			if (reader->getNodeType() == io::EXN_ELEMENT)
			{
				if (assetSectionName == reader->getNodeName())
					skipSection(reader, false);
				else
				if (initFromName == reader->getNodeName())
				{
					reader->read();
					image.Source = reader->getNodeData();
					image.Source.trim();
					image.SourceIsFilename=true;
				}
				else
				if (dataName == reader->getNodeName())
				{
					reader->read();
					image.Source = reader->getNodeData();
					image.Source.trim();
					image.SourceIsFilename=false;
				}
				else
				if (extraNodeName == reader->getNodeName())
					skipSection(reader, false);
			}
			else
			if (reader->getNodeType() == io::EXN_ELEMENT_END)
			{
				if (initFromName == reader->getNodeName())
					return;
			}
		}
	}
	else
	{
		image.Source = reader->getAttributeValue("source");
		image.Source.trim();
		image.SourceIsFilename=false;
	}
}


//! reads a <texture> element and stores it in the texture section
void CColladaFileLoader::readTexture(io::IXMLReaderUTF8* reader)
{
	// add texture to list of loaded textures.
	Textures.push_back(SColladaTexture());
	SColladaTexture& texture=Textures.getLast();

	texture.Id = readId(reader);
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading texture", core::stringc(texture.Id), ELL_DEBUG);
	#endif

	if (!reader->isEmptyElement())
	{
		readColladaInputs(reader, textureSectionName);
		SColladaInput* input = getColladaInput(ECIS_IMAGE);
		if (input)
		{
			const core::stringc imageName = input->Source;
			texture.Texture = getTextureFromImage(imageName, NULL);
		}
	}
}


//! reads a <material> element and stores it in the material section
void CColladaFileLoader::readMaterial(io::IXMLReaderUTF8* reader)
{
	// add material to list of loaded materials.
	Materials.push_back(SColladaMaterial());

	SColladaMaterial& material = Materials.getLast();
	material.Id = readId(reader);
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading material", core::stringc(material.Id), ELL_DEBUG);
	#endif

	if (Version >= 10400)
	{
		while(reader->read())
		{
			if (reader->getNodeType() == io::EXN_ELEMENT &&
				instanceEffectName == reader->getNodeName())
			{
				material.InstanceEffectId = reader->getAttributeValue("url");
				uriToId(material.InstanceEffectId);
			}
			else
			if (reader->getNodeType() == io::EXN_ELEMENT_END &&
				materialSectionName == reader->getNodeName())
			{
				break;
			}
		} // end while reader->read();
	}
	else
	{
		if (!reader->isEmptyElement())
		{
			readColladaInputs(reader, materialSectionName);
			SColladaInput* input = getColladaInput(ECIS_TEXTURE);
			if (input)
			{
				core::stringc textureName = input->Source;
				uriToId(textureName);
				for (u32 i=0; i<Textures.size(); ++i)
					if (textureName == Textures[i].Id)
					{
						material.Mat.setTexture(0, Textures[i].Texture);
						break;
					}
			}

			//does not work because the wrong start node is chosen due to reading of inputs before
#if 0
			readColladaParameters(reader, materialSectionName);

			SColladaParam* p;

			p = getColladaParameter(ECPN_AMBIENT);
			if (p && p->Type == ECPT_FLOAT3)
				material.Mat.AmbientColor = video::SColorf(p->Floats[0],p->Floats[1],p->Floats[2]).toSColor();
			p = getColladaParameter(ECPN_DIFFUSE);
			if (p && p->Type == ECPT_FLOAT3)
				material.Mat.DiffuseColor = video::SColorf(p->Floats[0],p->Floats[1],p->Floats[2]).toSColor();
			p = getColladaParameter(ECPN_SPECULAR);
			if (p && p->Type == ECPT_FLOAT3)
				material.Mat.DiffuseColor = video::SColorf(p->Floats[0],p->Floats[1],p->Floats[2]).toSColor();
			p = getColladaParameter(ECPN_SHININESS);
			if (p && p->Type == ECPT_FLOAT)
				material.Mat.Shininess = p->Floats[0];
#endif
		}
	}
}

void CColladaFileLoader::readEffect(io::IXMLReaderUTF8* reader, SColladaEffect * effect)
{
	static const core::stringc constantNode("constant");
	static const core::stringc lambertNode("lambert");
	static const core::stringc phongNode("phong");
	static const core::stringc blinnNode("blinn");
	static const core::stringc emissionNode("emission");
	static const core::stringc ambientNode("ambient");
	static const core::stringc diffuseNode("diffuse");
	static const core::stringc specularNode("specular");
	static const core::stringc shininessNode("shininess");
	static const core::stringc reflectiveNode("reflective");
	static const core::stringc reflectivityNode("reflectivity");
	static const core::stringc transparentNode("transparent");
	static const core::stringc transparencyNode("transparency");
	static const core::stringc indexOfRefractionNode("index_of_refraction");

	if (!effect)
	{
		Effects.push_back(SColladaEffect());
		effect = &Effects.getLast();
		effect->Parameters = new io::CAttributes();
		effect->Id = readId(reader);
		effect->Transparency = 1.f;
		effect->Mat.Lighting=true;
		effect->Mat.NormalizeNormals=true;
		#ifdef COLLADA_READER_DEBUG
		os::Printer::log("COLLADA reading effect", core::stringc(effect->Id), ELL_DEBUG);
		#endif
	}
	while(reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT)
		{
			// first come the tags we descend, but ignore the top-levels
			if (!reader->isEmptyElement() && ((profileCOMMONSectionName == reader->getNodeName()) ||
				(techniqueNodeName == reader->getNodeName())))
				readEffect(reader,effect);
			else
			if (newParamName == reader->getNodeName())
				readParameter(reader, effect->Parameters);
			else
			// these are the actual materials inside technique
			if (constantNode == reader->getNodeName() ||
				lambertNode == reader->getNodeName() ||
				phongNode == reader->getNodeName() ||
				blinnNode == reader->getNodeName())
			{
				#ifdef COLLADA_READER_DEBUG
				os::Printer::log("COLLADA reading effect part", reader->getNodeName(), ELL_DEBUG);
				#endif
				effect->Mat.setFlag(irr::video::EMF_GOURAUD_SHADING,
					phongNode == reader->getNodeName() ||
					blinnNode == reader->getNodeName());
				while(reader->read())
				{
					if (reader->getNodeType() == io::EXN_ELEMENT)
					{
						const core::stringc node = reader->getNodeName();
						if (emissionNode == node || ambientNode == node ||
							diffuseNode == node || specularNode == node ||
							reflectiveNode == node || transparentNode == node )
						{
							// color or texture types
							while(reader->read())
							{
								if (reader->getNodeType() == io::EXN_ELEMENT &&
									colorNodeName == reader->getNodeName())
								{
									const video::SColorf colorf = readColorNode(reader);
									const video::SColor color = colorf.toSColor();
									if (emissionNode == node)
										effect->Mat.EmissiveColor = color;
									else
									if (ambientNode == node)
										effect->Mat.AmbientColor = color;
									else
									if (diffuseNode == node)
										effect->Mat.DiffuseColor = color;
									else
									if (specularNode == node)
										effect->Mat.SpecularColor = color;
									else
									if (transparentNode == node)
										effect->Transparency = colorf.getAlpha();
								}
								else
								if (reader->getNodeType() == io::EXN_ELEMENT &&
									textureNodeName == reader->getNodeName())
								{
									effect->Textures.push_back(reader->getAttributeValue("texture"));
									break;
								}
								else
								if (reader->getNodeType() == io::EXN_ELEMENT)
									skipSection(reader, false);
								else
								if (reader->getNodeType() == io::EXN_ELEMENT_END &&
									node == reader->getNodeName())
									break;
							}
						}
						else
						if (shininessNode == node || reflectivityNode == node ||
							transparencyNode == node || indexOfRefractionNode == node )
						{
							// float or param types
							while(reader->read())
							{
								if (reader->getNodeType() == io::EXN_ELEMENT &&
									floatNodeName == reader->getNodeName())
								{
									f32 f = readFloatNode(reader);
									if (shininessNode == node)
										effect->Mat.Shininess = f;
									else
									if (transparencyNode == node)
										effect->Transparency *= f;
								}
								else
								if (reader->getNodeType() == io::EXN_ELEMENT)
									skipSection(reader, false);
								else
								if (reader->getNodeType() == io::EXN_ELEMENT_END &&
									node == reader->getNodeName())
									break;
							}
						}
						else
							skipSection(reader, true); // ignore all other nodes
					}
					else
					if (reader->getNodeType() == io::EXN_ELEMENT_END && (
						constantNode == reader->getNodeName() ||
						lambertNode == reader->getNodeName() ||
						phongNode == reader->getNodeName() ||
						blinnNode == reader->getNodeName()
						))
						break;
				}
			}
			else
			if (!reader->isEmptyElement() && (extraNodeName == reader->getNodeName()))
				readEffect(reader,effect);
			else
			if (doubleSidedNodeName == reader->getNodeName())
			{
				// read the GoogleEarth extra flag for double sided polys
				s32 doubleSided = 0;
				readIntsInsideElement(reader,&doubleSided,1);
				if (doubleSided)
				{
					#ifdef COLLADA_READER_DEBUG
					os::Printer::log("Setting double sided flag for effect.", ELL_DEBUG);
					#endif

					effect->Mat.setFlag(irr::video::EMF_BACK_FACE_CULLING,false);
				}
			}
			else
				skipSection(reader, true); // ignore all other sections
		}
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
		{
			if (effectSectionName == reader->getNodeName())
				break;
			else
			if (profileCOMMONSectionName == reader->getNodeName())
				break;
			else
			if (techniqueNodeName == reader->getNodeName())
				break;
			else
			if (extraNodeName == reader->getNodeName())
				break;
		}
	}

	if (effect->Mat.AmbientColor == video::SColor(0) &&
		effect->Mat.DiffuseColor != video::SColor(0))
		effect->Mat.AmbientColor = effect->Mat.DiffuseColor;
	if (effect->Mat.DiffuseColor == video::SColor(0) &&
		effect->Mat.AmbientColor != video::SColor(0))
		effect->Mat.DiffuseColor = effect->Mat.AmbientColor;
	if ((effect->Transparency != 0.0f) && (effect->Transparency != 1.0f))
	{
		effect->Mat.MaterialType = irr::video::EMT_TRANSPARENT_VERTEX_ALPHA;
		effect->Mat.ZWriteEnable = false;
	}

	video::E_TEXTURE_CLAMP twu = video::ETC_REPEAT;
	s32 idx = effect->Parameters->findAttribute(wrapsName.c_str());
	if ( idx >= 0 )
		twu = (video::E_TEXTURE_CLAMP)(effect->Parameters->getAttributeAsInt(idx));
	video::E_TEXTURE_CLAMP twv = video::ETC_REPEAT;
	idx = effect->Parameters->findAttribute(wraptName.c_str());
	if ( idx >= 0 )
		twv = (video::E_TEXTURE_CLAMP)(effect->Parameters->getAttributeAsInt(idx));
	
	for (u32 i=0; i<video::MATERIAL_MAX_TEXTURES; ++i)
	{
		effect->Mat.TextureLayer[i].TextureWrapU = twu;
		effect->Mat.TextureLayer[i].TextureWrapV = twv;
	}

	effect->Mat.setFlag(video::EMF_BILINEAR_FILTER, effect->Parameters->getAttributeAsBool("bilinear"));
	effect->Mat.setFlag(video::EMF_TRILINEAR_FILTER, effect->Parameters->getAttributeAsBool("trilinear"));
	effect->Mat.setFlag(video::EMF_ANISOTROPIC_FILTER, effect->Parameters->getAttributeAsBool("anisotropic"));
}


const SColladaMaterial* CColladaFileLoader::findMaterial(const core::stringc& materialName)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA find material", materialName, ELL_DEBUG);
	#endif

	// do a quick lookup in the materials
	SColladaMaterial matToFind;
	matToFind.Id = materialName;
	s32 mat = Materials.binary_search(matToFind);
	if (mat == -1)
		return 0;
	// instantiate the material effect if needed
	if (Materials[mat].InstanceEffectId.size() != 0)
	{
		// do a quick lookup in the effects
		SColladaEffect effectToFind;
		effectToFind.Id = Materials[mat].InstanceEffectId;
		s32 effect = Effects.binary_search(effectToFind);
		if (effect != -1)
		{
			// found the effect, instantiate by copying into the material
			Materials[mat].Mat = Effects[effect].Mat;
			if (Effects[effect].Textures.size())
				Materials[mat].Mat.setTexture(0, getTextureFromImage(Effects[effect].Textures[0], &(Effects[effect])));
			Materials[mat].Transparency = Effects[effect].Transparency;
			// and indicate the material is instantiated by removing the effect ref
			Materials[mat].InstanceEffectId = "";
		}
		else
			return 0;
	}
	return &Materials[mat];
}


void CColladaFileLoader::readBindMaterialSection(io::IXMLReaderUTF8* reader, const core::stringc & id)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading bind material", ELL_DEBUG);
	#endif

	while(reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT)
		{
			if (instanceMaterialName == reader->getNodeName())
			{
				// the symbol to retarget, and the target material
				core::stringc meshbufferReference = reader->getAttributeValue("symbol");
				if (meshbufferReference.size()==0)
					continue;
				core::stringc target = reader->getAttributeValue("target");
				uriToId(target);
				if (target.size()==0)
					continue;
				const SColladaMaterial * material = findMaterial(target);
				if (!material)
					continue;
				// bind any pending materials for this node
				meshbufferReference = id+"/"+meshbufferReference;
#ifdef COLLADA_READER_DEBUG
				os::Printer::log((core::stringc("Material binding: ")+meshbufferReference+" "+target).c_str(), ELL_DEBUG);
#endif
				if (MaterialsToBind.find(meshbufferReference))
				{
					core::array<irr::scene::IMeshBuffer*> & toBind
						= MeshesToBind[MaterialsToBind[meshbufferReference]];
#ifdef COLLADA_READER_DEBUG
				os::Printer::log("Material binding now ",material->Id.c_str(), ELL_DEBUG);
				os::Printer::log("#meshbuffers",core::stringc(toBind.size()).c_str(), ELL_DEBUG);
#endif
					SMesh tmpmesh;
					for (u32 i = 0; i < toBind.size(); ++i)
					{
						toBind[i]->getMaterial() = material->Mat;
						tmpmesh.addMeshBuffer(toBind[i]);

						if ((material->Transparency!=0.0f) && (material->Transparency!=1.0f))
						{
							toBind[i]->getMaterial().MaterialType = video::EMT_TRANSPARENT_VERTEX_ALPHA;
							toBind[i]->getMaterial().ZWriteEnable = false;
						}
					}
					SceneManager->getMeshManipulator()->setVertexColors(&tmpmesh,material->Mat.DiffuseColor);
					if ((material->Transparency!=0.0f) && (material->Transparency!=1.0f))
					{
						#ifdef COLLADA_READER_DEBUG
						os::Printer::log("COLLADA found transparency material", core::stringc(material->Transparency).c_str(), ELL_DEBUG);
						#endif
						SceneManager->getMeshManipulator()->setVertexColorAlpha(&tmpmesh, core::floor32(material->Transparency*255.0f));
					}
				}
			}
		}
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END &&
			bindMaterialName == reader->getNodeName())
			break;
	}
}


//! reads a <geometry> element and stores it as mesh if possible
void CColladaFileLoader::readGeometry(io::IXMLReaderUTF8* reader)
{
	core::stringc id = readId(reader);
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading geometry", id, ELL_DEBUG);
	#endif

	SAnimatedMesh* amesh = new SAnimatedMesh();
	scene::SMesh* mesh = new SMesh();
	amesh->addMesh(mesh);
	core::array<SSource> sources;
	bool okToReadArray = false;

	// handles geometry node and the mesh children in this loop
	// read sources with arrays and accessor for each mesh
	if (!reader->isEmptyElement())
	while(reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT)
		{
			const char* nodeName = reader->getNodeName();
			if (meshSectionName == nodeName)
			{
				// inside a mesh section. Don't have to do anything here.
			}
			else
			if (sourceSectionName == nodeName)
			{
				// create a new source
				sources.push_back(SSource());
				sources.getLast().Id = readId(reader);

				#ifdef COLLADA_READER_DEBUG
				os::Printer::log("Reading source", sources.getLast().Id.c_str(), ELL_DEBUG);
				#endif
			}
			else
			if (arraySectionName == nodeName || floatArraySectionName == nodeName || intArraySectionName == nodeName)
			{
				// create a new array and read it.
				if (!sources.empty())
				{
					sources.getLast().Array.Name = readId(reader);

					int count = reader->getAttributeValueAsInt("count");
					sources.getLast().Array.Data.set_used(count); // pre allocate

					// check if type of array is ok
					const char* type = reader->getAttributeValue("type");
					okToReadArray = (type && (!strcmp("float", type) || !strcmp("int", type))) || floatArraySectionName == nodeName || intArraySectionName == nodeName;

					#ifdef COLLADA_READER_DEBUG
					os::Printer::log("Read array", sources.getLast().Array.Name.c_str(), ELL_DEBUG);
					#endif
				}
				#ifdef COLLADA_READER_DEBUG
				else
					os::Printer::log("Warning, array outside source found",
						readId(reader).c_str(), ELL_DEBUG);
				#endif

			}
			else
			if (accessorSectionName == nodeName) // child of source (below a technique tag)
			{
				#ifdef COLLADA_READER_DEBUG
				os::Printer::log("Reading accessor", ELL_DEBUG);
				#endif
				SAccessor accessor;
				accessor.Count = reader->getAttributeValueAsInt("count");
				accessor.Offset = reader->getAttributeValueAsInt("offset");
				accessor.Stride = reader->getAttributeValueAsInt("stride");
				if (accessor.Stride == 0)
					accessor.Stride = 1;

				// the accessor contains some information on how to access (boi!) the array,
				// the info is stored in collada style parameters, so just read them.
				readColladaParameters(reader, accessorSectionName);
				if (!sources.empty())
				{
					sources.getLast().Accessors.push_back(accessor);
					sources.getLast().Accessors.getLast().Parameters = ColladaParameters;
				}
			}
			else
			if (verticesSectionName == nodeName)
			{
				#ifdef COLLADA_READER_DEBUG
				os::Printer::log("Reading vertices", ELL_DEBUG);
				#endif
				// read vertex input position source
				readColladaInputs(reader, verticesSectionName);
			}
			else
			// lines and linestrips missing
			if (polygonsSectionName == nodeName ||
				polylistSectionName == nodeName ||
				trianglesSectionName == nodeName)
			{
				// read polygons section
				readPolygonSection(reader, sources, mesh, id);
			}
			else
			// trifans, and tristrips missing
			if (doubleSidedNodeName == reader->getNodeName())
			{
				// read the extra flag for double sided polys
				s32 doubleSided = 0;
				readIntsInsideElement(reader,&doubleSided,1);
				if (doubleSided)
				{
					#ifdef COLLADA_READER_DEBUG
					os::Printer::log("Setting double sided flag for mesh.", ELL_DEBUG);
					#endif
					amesh->setMaterialFlag(irr::video::EMF_BACK_FACE_CULLING,false);
				}
			}
			else
			 // techniqueCommon or 'technique profile=common' must not be skipped
			if ((techniqueCommonSectionName != nodeName) // Collada 1.2/1.3
				&& (techniqueNodeName != nodeName) // Collada 1.4+
				&& (extraNodeName != nodeName))
			{
				os::Printer::log("COLLADA loader warning: Wrong tag usage found in geometry", reader->getNodeName(), ELL_WARNING);
				skipSection(reader, true); // ignore all other sections
			}
		} // end if node type is element
		else
		if (reader->getNodeType() == io::EXN_TEXT)
		{
			// read array data
			if (okToReadArray && !sources.empty())
			{
				core::array<f32>& a = sources.getLast().Array.Data;
				core::stringc data = reader->getNodeData();
				data.trim();
				const c8* p = &data[0];

				for (u32 i=0; i<a.size(); ++i)
				{
					findNextNoneWhiteSpace(&p);
					if (*p)
						a[i] = readFloat(&p);
					else
						a[i] = 0.0f;
				}
			} // end reading array

			okToReadArray = false;

		} // end if node type is text
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
		{
			if (geometrySectionName == reader->getNodeName())
			{
				// end of geometry section reached, cancel out
				break;
			}
		}
	} // end while reader->read();

	// add mesh as geometry

	mesh->recalculateBoundingBox();
	amesh->recalculateBoundingBox();

	// create virtual file name
	io::path filename = CurrentlyLoadingMesh;
	filename += '#';
	filename += id;

	// add to scene manager
	if (LoadedMeshCount)
	{
		SceneManager->getMeshCache()->addMesh(filename.c_str(), amesh);
		os::Printer::log("Added COLLADA mesh", filename.c_str(), ELL_DEBUG);
	}
	else
	{
		FirstLoadedMeshName = filename;
		FirstLoadedMesh = amesh;
		FirstLoadedMesh->grab();
	}

	++LoadedMeshCount;
	mesh->drop();
	amesh->drop();

	// create geometry prefab
	u32 i;
	for (i=0; i<Prefabs.size(); ++i)
	{
		if (Prefabs[i]->getId()==id)
		{
			((CGeometryPrefab*)Prefabs[i])->Mesh=mesh;
			break;
		}
	}
	if (i==Prefabs.size())
	{
		CGeometryPrefab* prefab = new CGeometryPrefab(id);
		prefab->Mesh = mesh;
		Prefabs.push_back(prefab);
	}

	// store as dummy mesh if no instances will be created
	if (!CreateInstances && !DummyMesh)
	{
		DummyMesh = amesh;
		DummyMesh->grab();
	}
}


struct SPolygon
{
	core::array<s32> Indices;
};

//! reads a polygons section and creates a mesh from it
void CColladaFileLoader::readPolygonSection(io::IXMLReaderUTF8* reader,
		core::array<SSource>& sources, scene::SMesh* mesh,
		const core::stringc& geometryId)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading polygon section", ELL_DEBUG);
	#endif

	core::stringc materialName = reader->getAttributeValue("material");

	core::stringc polygonType = reader->getNodeName();
	const int polygonCount = reader->getAttributeValueAsInt("count"); // Not useful because it only determines the number of primitives, which have arbitrary vertices in case of polygon
	core::array<SPolygon> polygons;
	if (polygonType == polygonsSectionName)
		polygons.reallocate(polygonCount);
	core::array<int> vCounts;
	bool parsePolygonOK = false;
	bool parseVcountOK = false;
	u32 inputSemanticCount = 0;
	bool unresolvedInput=false;
	u32 maxOffset = 0;
	core::array<SColladaInput> localInputs;

	// read all <input> and primitives
	if (!reader->isEmptyElement())
	while(reader->read())
	{
		const char* nodeName = reader->getNodeName();

		if (reader->getNodeType() == io::EXN_ELEMENT)
		{
			// polygon node may contain params
			if (inputTagName == nodeName)
			{
				// read input tag
				readColladaInput(reader, localInputs);

				// resolve input source
				SColladaInput& inp = localInputs.getLast();

				// get input source array id, if it is a vertex input, take
				// the <vertex><input>-source attribute.
				if (inp.Semantic == ECIS_VERTEX)
				{
					inp.Source = Inputs[0].Source;
					for (u32 i=1; i<Inputs.size(); ++i)
					{
						localInputs.push_back(Inputs[i]);
						uriToId(localInputs.getLast().Source);
						maxOffset = core::max_(maxOffset,localInputs.getLast().Offset);
						++inputSemanticCount;
					}
				}
				uriToId(inp.Source);
				maxOffset = core::max_(maxOffset,inp.Offset);
				++inputSemanticCount;
			}
			else
			if (primitivesName == nodeName)
			{
				parsePolygonOK = true;
				polygons.push_back(SPolygon());
			}
			else
			if (vcountName == nodeName)
			{
				parseVcountOK = true;
			} // end  is polygon node
		} // end is element node
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
		{
			if (primitivesName == nodeName)
				parsePolygonOK = false; // end parsing a polygon
			else
			if (vcountName == nodeName)
				parseVcountOK = false; // end parsing vcounts
			else
			if (polygonType == nodeName)
				break; // cancel out and create mesh

		} // end is element end
		else
		if (reader->getNodeType() == io::EXN_TEXT)
		{
			if (parseVcountOK)
			{
				core::stringc data = reader->getNodeData();
				data.trim();
				const c8* p = &data[0];
				while(*p)
				{
					findNextNoneWhiteSpace(&p);
					if (*p)
						vCounts.push_back(readInt(&p));
				}
				parseVcountOK = false;
			}
			else
			if (parsePolygonOK && polygons.size())
			{
				core::stringc data = reader->getNodeData();
				data.trim();
				const c8* p = &data[0];
				SPolygon& poly = polygons.getLast();
				if (polygonType == polygonsSectionName)
					poly.Indices.reallocate((maxOffset+1)*3);
				else
					poly.Indices.reallocate(polygonCount*(maxOffset+1)*3);

				if (vCounts.empty())
				{
					while(*p)
					{
						findNextNoneWhiteSpace(&p);
						poly.Indices.push_back(readInt(&p));
					}
				}
				else
				{
					for (u32 i = 0; i < vCounts.size(); i++)
					{
						const int polyVCount = vCounts[i];
						core::array<int> polyCorners;

						for (u32 j = 0; j < polyVCount * inputSemanticCount; j++)
						{
							if (!*p)
								break;
							findNextNoneWhiteSpace(&p);
							polyCorners.push_back(readInt(&p));
						}

						while (polyCorners.size() >= 3 * inputSemanticCount)
						{
							// add one triangle's worth of indices
							for (u32 k = 0; k < inputSemanticCount * 3; ++k)
							{
								poly.Indices.push_back(polyCorners[k]);
							}

							// remove one corner from our poly
							polyCorners.erase(inputSemanticCount,inputSemanticCount);
						}
						polyCorners.clear();
					}
					vCounts.clear();
				}
				parsePolygonOK = false;
			}
		}
	} // end while reader->read()

	// find source array (we'll ignore accessors for this implementation)
	for (u32 i=0; i<localInputs.size(); ++i)
	{
		SColladaInput& inp = localInputs[i];
		u32 s;
		for (s=0; s<sources.size(); ++s)
		{
			if (sources[s].Id == inp.Source)
			{
				// slot found
				inp.Data = sources[s].Array.Data.pointer();
				inp.Stride = sources[s].Accessors[0].Stride;
				break;
			}
		}

		if (s == sources.size())
		{
			os::Printer::log("COLLADA Warning, polygon input source not found",
				inp.Source.c_str(), ELL_DEBUG);
			inp.Semantic=ECIS_COUNT; // for unknown
			unresolvedInput=true;
		}
		else
		{
			#ifdef COLLADA_READER_DEBUG
			// print slot
			core::stringc tmp = "Added slot ";
			tmp += inputSemanticNames[inp.Semantic];
			tmp += " sourceArray:";
			tmp += inp.Source;
			os::Printer::log(tmp.c_str(), ELL_DEBUG);
			#endif
		}
	}

	if ((inputSemanticCount == 0) || !polygons.size())
		return; // cancel if there are no polygons anyway.

	// analyze content of Inputs to create a fitting mesh buffer

	u32 u;
	u32 textureCoordSetCount = 0;
	bool normalSlotCount = false;
	u32 secondTexCoordSetIndex = 0xFFFFFFFF;

	for (u=0; u<Inputs.size(); ++u)
	{
		if (Inputs[u].Semantic == ECIS_TEXCOORD || Inputs[u].Semantic == ECIS_UV )
		{
			++textureCoordSetCount;

			if (textureCoordSetCount==2)
				secondTexCoordSetIndex = u;
		}
		else
		if (Inputs[u].Semantic == ECIS_NORMAL)
			normalSlotCount=true;
	}

	// if there is more than one texture coordinate set, create a lightmap mesh buffer,
	// otherwise use a standard mesh buffer

	scene::IMeshBuffer* buffer = 0;
	++maxOffset; // +1 to jump to the next value

	if ( textureCoordSetCount < 2 )
	{
		// standard mesh buffer

		scene::SMeshBuffer* mbuffer = new SMeshBuffer();
		buffer = mbuffer;

		core::map<video::S3DVertex, int> vertMap;

		for (u32 i=0; i<polygons.size(); ++i)
		{
			core::array<u16> indices;
			const u32 vertexCount = polygons[i].Indices.size() / maxOffset;
			mbuffer->Vertices.reallocate(mbuffer->Vertices.size()+vertexCount);

			// for all index/semantic groups
			for (u32 v=0; v<polygons[i].Indices.size(); v+=maxOffset)
			{
				video::S3DVertex vtx;
				vtx.Color.set(255,255,255,255);

				// for all input semantics
				for (u32 k=0; k<localInputs.size(); ++k)
				{
					if (!localInputs[k].Data)
						continue;
					// build vertex from input semantics.

					const u32 idx = localInputs[k].Stride*polygons[i].Indices[v+localInputs[k].Offset];

					switch(localInputs[k].Semantic)
					{
					case ECIS_POSITION:
					case ECIS_VERTEX:
						vtx.Pos.X = localInputs[k].Data[idx+0];
						if (FlipAxis)
						{
							vtx.Pos.Z = localInputs[k].Data[idx+1];
							vtx.Pos.Y = localInputs[k].Data[idx+2];
						}
						else
						{
							vtx.Pos.Y = localInputs[k].Data[idx+1];
							vtx.Pos.Z = localInputs[k].Data[idx+2];
						}
						break;
					case ECIS_NORMAL:
						vtx.Normal.X = localInputs[k].Data[idx+0];
						if (FlipAxis)
						{
							vtx.Normal.Z = localInputs[k].Data[idx+1];
							vtx.Normal.Y = localInputs[k].Data[idx+2];
						}
						else
						{
							vtx.Normal.Y = localInputs[k].Data[idx+1];
							vtx.Normal.Z = localInputs[k].Data[idx+2];
						}
						break;
					case ECIS_TEXCOORD:
					case ECIS_UV:
						vtx.TCoords.X = localInputs[k].Data[idx+0];
						vtx.TCoords.Y = 1-localInputs[k].Data[idx+1];
						break;
					case ECIS_TANGENT:
						break;
					default:
						break;
					}
				}

				//first, try to find this vertex in the mesh
				core::map<video::S3DVertex, int>::Node* n = vertMap.find(vtx);
				if (n)
				{
					indices.push_back(n->getValue());
				}
				else
				{
					indices.push_back(mbuffer->getVertexCount());
					mbuffer->Vertices.push_back(vtx);
					vertMap.insert(vtx, mbuffer->getVertexCount()-1);
				}
			} // end for all vertices

			if (polygonsSectionName == polygonType &&
				indices.size() > 3)
			{
				// need to tesselate for polygons of 4 or more vertices
				// for now we naively turn interpret it as a triangle fan
				// as full tesselation is problematic
				if (FlipAxis)
				{
					for (u32 ind = indices.size()-3; ind>0 ; --ind)
					{
						mbuffer->Indices.push_back(indices[0]);
						mbuffer->Indices.push_back(indices[ind+2]);
						mbuffer->Indices.push_back(indices[ind+1]);
					}
				}
				else
				{
					for (u32 ind = 0; ind+2 < indices.size(); ++ind)
					{
						mbuffer->Indices.push_back(indices[0]);
						mbuffer->Indices.push_back(indices[ind+1]);
						mbuffer->Indices.push_back(indices[ind+2]);
					}
				}
			}
			else
			{
				// it's just triangles
				for (u32 ind = 0; ind < indices.size(); ind+=3)
				{
					if (FlipAxis)
					{
						mbuffer->Indices.push_back(indices[ind+2]);
						mbuffer->Indices.push_back(indices[ind+1]);
						mbuffer->Indices.push_back(indices[ind+0]);
					}
					else
					{
						mbuffer->Indices.push_back(indices[ind+0]);
						mbuffer->Indices.push_back(indices[ind+1]);
						mbuffer->Indices.push_back(indices[ind+2]);
					}
				}
			}

		} // end for all polygons
	}
	else
	{
		// lightmap mesh buffer

		scene::SMeshBufferLightMap* mbuffer = new SMeshBufferLightMap();
		buffer = mbuffer;

		for (u32 i=0; i<polygons.size(); ++i)
		{
			const u32 vertexCount = polygons[i].Indices.size() / maxOffset;
			mbuffer->Vertices.reallocate(mbuffer->Vertices.size()+vertexCount);
			// for all vertices in array
			for (u32 v=0; v<polygons[i].Indices.size(); v+=maxOffset)
			{
				video::S3DVertex2TCoords vtx;
				vtx.Color.set(100,255,255,255);

				// for all input semantics
				for (u32 k=0; k<Inputs.size(); ++k)
				{
					// build vertex from input semantics.

					const u32 idx = localInputs[k].Stride*polygons[i].Indices[v+Inputs[k].Offset];

					switch(localInputs[k].Semantic)
					{
					case ECIS_POSITION:
					case ECIS_VERTEX:
						vtx.Pos.X = localInputs[k].Data[idx+0];
						if (FlipAxis)
						{
							vtx.Pos.Z = localInputs[k].Data[idx+1];
							vtx.Pos.Y = localInputs[k].Data[idx+2];
						}
						else
						{
							vtx.Pos.Y = localInputs[k].Data[idx+1];
							vtx.Pos.Z = localInputs[k].Data[idx+2];
						}
						break;
					case ECIS_NORMAL:
						vtx.Normal.X = localInputs[k].Data[idx+0];
						if (FlipAxis)
						{
							vtx.Normal.Z = localInputs[k].Data[idx+1];
							vtx.Normal.Y = localInputs[k].Data[idx+2];
						}
						else
						{
							vtx.Normal.Y = localInputs[k].Data[idx+1];
							vtx.Normal.Z = localInputs[k].Data[idx+2];
						}
						break;
					case ECIS_TEXCOORD:
					case ECIS_UV:
						if (k==secondTexCoordSetIndex)
						{
							vtx.TCoords2.X = localInputs[k].Data[idx+0];
							vtx.TCoords2.Y = 1-localInputs[k].Data[idx+1];
						}
						else
						{
							vtx.TCoords.X = localInputs[k].Data[idx+0];
							vtx.TCoords.Y = 1-localInputs[k].Data[idx+1];
						}
						break;
					case ECIS_TANGENT:
						break;
					default:
						break;
					}
				}

				mbuffer->Vertices.push_back(vtx);

			} // end for all vertices

			// add vertex indices
			const u32 oldVertexCount = mbuffer->Vertices.size() - vertexCount;
			for (u32 face=0; face<vertexCount-2; ++face)
			{
				mbuffer->Indices.push_back(oldVertexCount + 0);
				mbuffer->Indices.push_back(oldVertexCount + 1 + face);
				mbuffer->Indices.push_back(oldVertexCount + 2 + face);
			}

		} // end for all polygons
	}

	const SColladaMaterial* m = findMaterial(materialName);
	if (m)
	{
		buffer->getMaterial() = m->Mat;
		SMesh tmpmesh;
		tmpmesh.addMeshBuffer(buffer);
		SceneManager->getMeshManipulator()->setVertexColors(&tmpmesh,m->Mat.DiffuseColor);
		if (m->Transparency != 1.0f)
			SceneManager->getMeshManipulator()->setVertexColorAlpha(&tmpmesh,core::floor32(m->Transparency*255.0f));
	}
	// add future bind reference for the material
	core::stringc meshbufferReference = geometryId+"/"+materialName;
	if (!MaterialsToBind.find(meshbufferReference))
	{
		MaterialsToBind[meshbufferReference] = MeshesToBind.size();
		MeshesToBind.push_back(core::array<irr::scene::IMeshBuffer*>());
	}
	MeshesToBind[MaterialsToBind[meshbufferReference]].push_back(buffer);

	// calculate normals if there is no slot for it

	if (!normalSlotCount)
		SceneManager->getMeshManipulator()->recalculateNormals(buffer, true);

	// recalculate bounding box
	buffer->recalculateBoundingBox();

	// add mesh buffer
	mesh->addMeshBuffer(buffer);
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA added meshbuffer", core::stringc(buffer->getVertexCount())+" vertices, "+core::stringc(buffer->getIndexCount())+" indices.", ELL_DEBUG);
	#endif

	buffer->drop();
}


//! reads a <light> element and stores it as prefab
void CColladaFileLoader::readLightPrefab(io::IXMLReaderUTF8* reader)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading light prefab", ELL_DEBUG);
	#endif

	CLightPrefab* prefab = new CLightPrefab(readId(reader));

	if (!reader->isEmptyElement())
	{
		if (Version >= 10400) // start with 1.4
		{
			while(reader->read())
			{
				if (reader->getNodeType() == io::EXN_ELEMENT)
				{
					if (pointSectionName == reader->getNodeName())
						prefab->LightData.Type=video::ELT_POINT;
					else
					if (directionalSectionName == reader->getNodeName())
						prefab->LightData.Type=video::ELT_DIRECTIONAL;
					else
					if (spotSectionName == reader->getNodeName())
						prefab->LightData.Type=video::ELT_SPOT;
					else
					if (ambientSectionName == reader->getNodeName())
						prefab->LightData.Type=ELT_AMBIENT;
					else
					if (colorNodeName == reader->getNodeName())
						prefab->LightData.DiffuseColor=readColorNode(reader);
					else
					if (constantAttenuationNodeName == reader->getNodeName())
						readFloatsInsideElement(reader,&prefab->LightData.Attenuation.X,1);
					else
					if (linearAttenuationNodeName == reader->getNodeName())
						readFloatsInsideElement(reader,&prefab->LightData.Attenuation.Y,1);
					else
					if (quadraticAttenuationNodeName == reader->getNodeName())
						readFloatsInsideElement(reader,&prefab->LightData.Attenuation.Z,1);
					else
					if (falloffAngleNodeName == reader->getNodeName())
					{
						readFloatsInsideElement(reader,&prefab->LightData.OuterCone,1);
						prefab->LightData.OuterCone *= core::DEGTORAD;
					}
					else
					if (falloffExponentNodeName == reader->getNodeName())
						readFloatsInsideElement(reader,&prefab->LightData.Falloff,1);
				}
				else
				if (reader->getNodeType() == io::EXN_ELEMENT_END)
				{
					if ((pointSectionName == reader->getNodeName()) ||
						(directionalSectionName == reader->getNodeName()) ||
						(spotSectionName == reader->getNodeName()) ||
						(ambientSectionName == reader->getNodeName()))
						break;
				}
			}
		}
		else
		{
			readColladaParameters(reader, lightPrefabName);

			SColladaParam* p = getColladaParameter(ECPN_COLOR);
			if (p && p->Type == ECPT_FLOAT3)
				prefab->LightData.DiffuseColor.set(p->Floats[0], p->Floats[1], p->Floats[2]);
		}
	}

	Prefabs.push_back(prefab);
}


//! returns a collada parameter or none if not found
SColladaParam* CColladaFileLoader::getColladaParameter(ECOLLADA_PARAM_NAME name)
{
	for (u32 i=0; i<ColladaParameters.size(); ++i)
		if (ColladaParameters[i].Name == name)
			return &ColladaParameters[i];

	return 0;
}

//! returns a collada input or none if not found
SColladaInput* CColladaFileLoader::getColladaInput(ECOLLADA_INPUT_SEMANTIC input)
{
	for (u32 i=0; i<Inputs.size(); ++i)
		if (Inputs[i].Semantic == input)
			return &Inputs[i];

	return 0;
}


//! reads a collada input tag and adds it to the input parameter
void CColladaFileLoader::readColladaInput(io::IXMLReaderUTF8* reader, core::array<SColladaInput>& inputs)
{
	// parse param
	SColladaInput p;

	// get type
	core::stringc semanticName = reader->getAttributeValue("semantic");
	for (u32 i=0; inputSemanticNames[i]; ++i)
	{
		if (semanticName == inputSemanticNames[i])
		{
			p.Semantic = (ECOLLADA_INPUT_SEMANTIC)i;
			break;
		}
	}

	// get source
	p.Source = reader->getAttributeValue("source");
	if (reader->getAttributeValue("offset")) // Collada 1.4+
		p.Offset = (u32)reader->getAttributeValueAsInt("offset");
	else // Collada 1.2/1.3
		p.Offset = (u32)reader->getAttributeValueAsInt("idx");
	p.Set = (u32)reader->getAttributeValueAsInt("set");

	// add input
	inputs.push_back(p);
}

//! parses all collada inputs inside an element and stores them in Inputs
void CColladaFileLoader::readColladaInputs(io::IXMLReaderUTF8* reader, const core::stringc& parentName)
{
	Inputs.clear();

	while(reader->read())
	{
		if (reader->getNodeType() == io::EXN_ELEMENT &&
			inputTagName == reader->getNodeName())
		{
			readColladaInput(reader, Inputs);
		}
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
		{
			if (parentName == reader->getNodeName())
				return; // end of parent reached
		}

	} // end while reader->read();
}

//! parses all collada parameters inside an element and stores them in ColladaParameters
void CColladaFileLoader::readColladaParameters(io::IXMLReaderUTF8* reader,
		const core::stringc& parentName)
{
	ColladaParameters.clear();

	const char* const paramNames[] = {"COLOR", "AMBIENT", "DIFFUSE",
		"SPECULAR", "SHININESS", "YFOV", "ZNEAR", "ZFAR", 0};

	const char* const typeNames[] = {"float", "float2", "float3", 0};

	while(reader->read())
	{
		const char* nodeName = reader->getNodeName();
		if (reader->getNodeType() == io::EXN_ELEMENT &&
			paramTagName == nodeName)
		{
			// parse param
			SColladaParam p;

			// get type
			u32 i;
			core::stringc typeName = reader->getAttributeValue("type");
			for (i=0; typeNames[i]; ++i)
				if (typeName == typeNames[i])
				{
					p.Type = (ECOLLADA_PARAM_TYPE)i;
					break;
				}

			// get name
			core::stringc nameName = reader->getAttributeValue("name");
			for (i=0; typeNames[i]; ++i)
				if (nameName == paramNames[i])
				{
					p.Name = (ECOLLADA_PARAM_NAME)i;
					break;
				}

			// read parameter data inside parameter tags
			switch(p.Type)
			{
				case ECPT_FLOAT:
				case ECPT_FLOAT2:
				case ECPT_FLOAT3:
				case ECPT_FLOAT4:
					readFloatsInsideElement(reader, p.Floats, p.Type - ECPT_FLOAT + 1);
					break;

				// TODO: other types of data (ints, bools or whatever)
				default:
					break;
			}

			// add param
			ColladaParameters.push_back(p);
		}
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
		{
			if (parentName == reader->getNodeName())
				return; // end of parent reached
		}

	} // end while reader->read();
}


//! parses a float from a char pointer and moves the pointer
//! to the end of the parsed float
inline f32 CColladaFileLoader::readFloat(const c8** p)
{
	f32 ftmp;
	*p = core::fast_atof_move(*p, ftmp);
	return ftmp;
}


//! parses an int from a char pointer and moves the pointer to
//! the end of the parsed float
inline s32 CColladaFileLoader::readInt(const c8** p)
{
	return (s32)readFloat(p);
}


//! places pointer to next begin of a token
void CColladaFileLoader::findNextNoneWhiteSpace(const c8** start)
{
	const c8* p = *start;

	while(*p && (*p==' ' || *p=='\n' || *p=='\r' || *p=='\t'))
		++p;

	// TODO: skip comments <!-- -->

	*start = p;
}


//! reads floats from inside of xml element until end of xml element
void CColladaFileLoader::readFloatsInsideElement(io::IXMLReaderUTF8* reader, f32* floats, u32 count)
{
	if (reader->isEmptyElement())
		return;

	while(reader->read())
	{
		// TODO: check for comments inside the element
		// and ignore them.

		if (reader->getNodeType() == io::EXN_TEXT)
		{
			// parse float data
			core::stringc data = reader->getNodeData();
			data.trim();
			const c8* p = &data[0];

			for (u32 i=0; i<count; ++i)
			{
				findNextNoneWhiteSpace(&p);
				if (*p)
					floats[i] = readFloat(&p);
				else
					floats[i] = 0.0f;
			}
		}
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
			break; // end parsing text
	}
}


//! reads ints from inside of xml element until end of xml element
void CColladaFileLoader::readIntsInsideElement(io::IXMLReaderUTF8* reader, s32* ints, u32 count)
{
	if (reader->isEmptyElement())
		return;

	while(reader->read())
	{
		// TODO: check for comments inside the element
		// and ignore them.

		if (reader->getNodeType() == io::EXN_TEXT)
		{
			// parse float data
			core::stringc data = reader->getNodeData();
			data.trim();
			const c8* p = &data[0];

			for (u32 i=0; i<count; ++i)
			{
				findNextNoneWhiteSpace(&p);
				if (*p)
					ints[i] = readInt(&p);
				else
					ints[i] = 0;
			}
		}
		else
		if (reader->getNodeType() == io::EXN_ELEMENT_END)
			break; // end parsing text
	}
}


video::SColorf CColladaFileLoader::readColorNode(io::IXMLReaderUTF8* reader)
{
	if (reader->getNodeType() == io::EXN_ELEMENT &&
		colorNodeName == reader->getNodeName())
	{
		f32 color[4];
		readFloatsInsideElement(reader,color,4);
		return video::SColorf(color[0], color[1], color[2], color[3]);
	}

	return video::SColorf();
}


f32 CColladaFileLoader::readFloatNode(io::IXMLReaderUTF8* reader)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading <float>", ELL_DEBUG);
	#endif

	f32 result = 0.0f;
	if (reader->getNodeType() == io::EXN_ELEMENT &&
		floatNodeName == reader->getNodeName())
	{
		readFloatsInsideElement(reader,&result,1);
	}

	return result;
}


//! clears all loaded data
void CColladaFileLoader::clearData()
{
	// delete all prefabs

	for (u32 i=0; i<Prefabs.size(); ++i)
		Prefabs[i]->drop();

	Prefabs.clear();

	// clear all parameters
	ColladaParameters.clear();

	// clear all materials
	Images.clear();

	// clear all materials
	Textures.clear();

	// clear all materials
	Materials.clear();

	// clear all inputs
	Inputs.clear();

	// clear all effects
	for ( u32 i=0; i<Effects.size(); ++i )
		Effects[i].Parameters->drop();
	Effects.clear();

	// clear all the materials to bind
	MaterialsToBind.clear();
	MeshesToBind.clear();
}


//! changes the XML URI into an internal id
void CColladaFileLoader::uriToId(core::stringc& str)
{
	// currently, we only remove the # from the begin if there
	// because we simply don't support referencing other files.
	if (!str.size())
		return;

	if (str[0] == '#')
		str.erase(0);
}


//! read Collada Id, uses id or name if id is missing
core::stringc CColladaFileLoader::readId(io::IXMLReaderUTF8* reader)
{
	core::stringc id = reader->getAttributeValue("id");
	if (id.size()==0)
		id = reader->getAttributeValue("name");
	return id;
}


//! create an Irrlicht texture from the reference
video::ITexture* CColladaFileLoader::getTextureFromImage(core::stringc uri, SColladaEffect * effect)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA searching texture", uri, ELL_DEBUG);
	#endif
	video::IVideoDriver* driver = SceneManager->getVideoDriver();
	for (;;)
	{
		uriToId(uri);
		for (u32 i=0; i<Images.size(); ++i)
		{
			if (uri == Images[i].Id)
			{
				if (Images[i].Source.size() && Images[i].SourceIsFilename)
				{
					if (FileSystem->existFile(Images[i].Source))
						return driver->getTexture(Images[i].Source);
					return driver->getTexture((FileSystem->getFileDir(CurrentlyLoadingMesh)+"/"+Images[i].Source));
				}
				else
				if (Images[i].Source.size())
				{
					//const u32 size = Images[i].Dimension.getArea();
					const u32 size = Images[i].Dimension.Width * Images[i].Dimension.Height;;
					u32* data = new u32[size]; // we assume RGBA
					u32* ptrdest = data;
					const c8* ptrsrc = Images[i].Source.c_str();
					for (u32 j=0; j<size; ++j)
					{
						sscanf(ptrsrc, "%x", ptrdest);
						++ptrdest;
						ptrsrc += 4;
					}
					video::IImage* img = driver->createImageFromData(video::ECF_A8R8G8B8, Images[i].Dimension, data, true, true);
					video::ITexture* tex = driver->addTexture((CurrentlyLoadingMesh+"#"+Images[i].Id).c_str(), img);
					img->drop();
					return tex;
				}
				break;
			}
		}
		if (effect && effect->Parameters->getAttributeType(uri.c_str())==io::EAT_STRING)
		{
			uri = effect->Parameters->getAttributeAsString(uri.c_str());
#ifdef COLLADA_READER_DEBUG
			os::Printer::log("COLLADA now searching texture", uri.c_str(), ELL_DEBUG);
#endif
		}
		else
			break;
	}
	return 0;
}


//! read a parameter and value
void CColladaFileLoader::readParameter(io::IXMLReaderUTF8* reader, io::IAttributes* parameters)
{
	#ifdef COLLADA_READER_DEBUG
	os::Printer::log("COLLADA reading parameter", ELL_DEBUG);
	#endif

	if ( !parameters )
		return;

	const core::stringc name = reader->getAttributeValue("sid");
	if (!reader->isEmptyElement())
	{
		while(reader->read())
		{
			if (reader->getNodeType() == io::EXN_ELEMENT)
			{
				if (floatNodeName == reader->getNodeName())
				{
					const f32 f = readFloatNode(reader);
					parameters->addFloat(name.c_str(), f);
				}
				else
				if (float2NodeName == reader->getNodeName())
				{
					f32 f[2];
					readFloatsInsideElement(reader, f, 2);
//						Parameters.addVector2d(name.c_str(), core::vector2df(f[0],f[1]));
				}
				else
				if (float3NodeName == reader->getNodeName())
				{
					f32 f[3];
					readFloatsInsideElement(reader, f, 3);
					parameters->addVector3d(name.c_str(), core::vector3df(f[0],f[1],f[2]));
				}
				else
				if ((initFromName == reader->getNodeName()) ||
					(sourceSectionName == reader->getNodeName()))
				{
					reader->read();
					parameters->addString(name.c_str(), reader->getNodeData());
				}
				else
				if (wrapsName == reader->getNodeName())
				{
					reader->read();
					const core::stringc val = reader->getNodeData();
					if (val == "WRAP")
						parameters->addInt(wrapsName.c_str(), (int)video::ETC_REPEAT);
					else if ( val== "MIRROR")
						parameters->addInt(wrapsName.c_str(), (int)video::ETC_MIRROR);
					else if ( val== "CLAMP")
						parameters->addInt(wrapsName.c_str(), (int)video::ETC_CLAMP_TO_EDGE);
					else if ( val== "BORDER")
						parameters->addInt(wrapsName.c_str(), (int)video::ETC_CLAMP_TO_BORDER);
					else if ( val== "NONE")
						parameters->addInt(wrapsName.c_str(), (int)video::ETC_CLAMP_TO_BORDER);
				}
				else
				if (wraptName == reader->getNodeName())
				{
					reader->read();
					const core::stringc val = reader->getNodeData();
					if (val == "WRAP")
						parameters->addInt(wraptName.c_str(), (int)video::ETC_REPEAT);
					else if ( val== "MIRROR")
						parameters->addInt(wraptName.c_str(), (int)video::ETC_MIRROR);
					else if ( val== "CLAMP")
						parameters->addInt(wraptName.c_str(), (int)video::ETC_CLAMP_TO_EDGE);
					else if ( val== "BORDER")
						parameters->addInt(wraptName.c_str(), (int)video::ETC_CLAMP_TO_BORDER);
					else if ( val== "NONE")
						parameters->addInt(wraptName.c_str(), (int)video::ETC_CLAMP_TO_BORDER);
				}
				else
				if (minfilterName == reader->getNodeName())
				{
					reader->read();
					const core::stringc val = reader->getNodeData();
					if (val == "LINEAR_MIPMAP_LINEAR")
						parameters->addBool("trilinear", true);
					else
					if (val == "LINEAR_MIPMAP_NEAREST")
						parameters->addBool("bilinear", true);
				}
				else
				if (magfilterName == reader->getNodeName())
				{
					reader->read();
					const core::stringc val = reader->getNodeData();
					if (val != "LINEAR")
					{
						parameters->addBool("bilinear", false);
						parameters->addBool("trilinear", false);
					}
				}
				else
				if (mipfilterName == reader->getNodeName())
				{
					parameters->addBool("anisotropic", true);
				}
			}
			else
			if(reader->getNodeType() == io::EXN_ELEMENT_END)
			{
				if (newParamName == reader->getNodeName())
					break;
			}
		}
	}
}


} // end namespace scene
} // end namespace irr

#endif // _IRR_COMPILE_WITH_COLLADA_LOADER_

