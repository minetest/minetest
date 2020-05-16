// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

// TODO: second UV-coordinates currently ignored in textures

#include "IrrCompileConfig.h"

#ifdef _IRR_COMPILE_WITH_COLLADA_WRITER_

#include "CColladaMeshWriter.h"
#include "os.h"
#include "IFileSystem.h"
#include "IWriteFile.h"
#include "IXMLWriter.h"
#include "IMesh.h"
#include "IAttributes.h"
#include "IAnimatedMeshSceneNode.h"
#include "IMeshSceneNode.h"
#include "ITerrainSceneNode.h"
#include "ILightSceneNode.h"
#include "ICameraSceneNode.h"
#include "ISceneManager.h"

namespace irr
{
namespace scene
{

//! Which lighting model should be used in the technique (FX) section when exporting effects (materials)
E_COLLADA_TECHNIQUE_FX CColladaMeshWriterProperties::getTechniqueFx(const video::SMaterial& material) const
{
	return ECTF_BLINN;
}

//! Which texture index should be used when writing the texture of the given sampler color.
s32 CColladaMeshWriterProperties::getTextureIdx(const video::SMaterial & material, E_COLLADA_COLOR_SAMPLER cs) const
{
	// So far we just export in a way which is similar to how we import colladas.
	// There might be better ways to do this, but I suppose it depends a lot for which target
	// application we export, so in most cases it will have to be done in user-code anyway.
	switch ( cs )
	{
		case ECCS_DIFFUSE:
			return 2;
		case ECCS_AMBIENT:
			return 1;
		case ECCS_EMISSIVE:
			return 0;
		case ECCS_SPECULAR:
			return 3;
		case ECCS_TRANSPARENT:
			return -1;
		case ECCS_REFLECTIVE:
			return -1;
	};
	return -1;
}

E_COLLADA_IRR_COLOR CColladaMeshWriterProperties::getColorMapping(const video::SMaterial & material, E_COLLADA_COLOR_SAMPLER cs) const
{
	switch ( cs )
	{
		case ECCS_DIFFUSE:
			return ECIC_DIFFUSE;
		case ECCS_AMBIENT:
			return ECIC_AMBIENT;
		case ECCS_EMISSIVE:
			return ECIC_EMISSIVE;
		case ECCS_SPECULAR:
			return ECIC_SPECULAR;
		case ECCS_TRANSPARENT:
			return ECIC_NONE;
		case ECCS_REFLECTIVE:
			return ECIC_CUSTOM;
	};

	return ECIC_NONE;
}

//! Return custom colors for certain color types requested by collada.
video::SColor CColladaMeshWriterProperties::getCustomColor(const video::SMaterial & material, E_COLLADA_COLOR_SAMPLER cs) const
{
	return video::SColor(255, 0, 0, 0);
}


//! Return the settings for transparence
E_COLLADA_TRANSPARENT_FX CColladaMeshWriterProperties::getTransparentFx(const video::SMaterial& material) const
{
	// TODO: figure out best default mapping
	return ECOF_A_ONE;
}

//! Transparency value for the material.
f32 CColladaMeshWriterProperties::getTransparency(const video::SMaterial& material) const
{
	// TODO: figure out best default mapping
	return -1.f;
}

//! Reflectivity value for that material
f32 CColladaMeshWriterProperties::getReflectivity(const video::SMaterial& material) const
{
	// TODO: figure out best default mapping
	return 0.f;
}

//! Return index of refraction for that material
f32 CColladaMeshWriterProperties::getIndexOfRefraction(const video::SMaterial& material) const
{
	return -1.f;
}

bool CColladaMeshWriterProperties::isExportable(const irr::scene::ISceneNode * node) const
{
	return node && node->isVisible();
}

IMesh* CColladaMeshWriterProperties::getMesh(irr::scene::ISceneNode * node)
{
	if ( !node )
		return 0;
	if ( node->getType() == ESNT_ANIMATED_MESH )
		return static_cast<IAnimatedMeshSceneNode*>(node)->getMesh()->getMesh(0);
	// TODO: we need some ISceneNode::hasType() function to get rid of those checks
	if (	node->getType() == ESNT_MESH
		||	node->getType() == ESNT_CUBE
		||	node->getType() == ESNT_SPHERE
		||	node->getType() == ESNT_WATER_SURFACE
		||	node->getType() == ESNT_Q3SHADER_SCENE_NODE
		)
		return static_cast<IMeshSceneNode*>(node)->getMesh();
	if ( node->getType() == ESNT_TERRAIN )
		return static_cast<ITerrainSceneNode*>(node)->getMesh();
	return 0;
}

// Check if the node has it's own material overwriting the mesh-materials
bool CColladaMeshWriterProperties::useNodeMaterial(const scene::ISceneNode* node) const
{
	if ( !node )
		return false;

	// TODO: we need some ISceneNode::hasType() function to get rid of those checks
	bool useMeshMaterial =	(	(node->getType() == ESNT_MESH ||
								node->getType() == ESNT_CUBE ||
								node->getType() == ESNT_SPHERE ||
								node->getType() == ESNT_WATER_SURFACE ||
								node->getType() == ESNT_Q3SHADER_SCENE_NODE)
								&& static_cast<const IMeshSceneNode*>(node)->isReadOnlyMaterials())

							||	(node->getType() == ESNT_ANIMATED_MESH
								&& static_cast<const IAnimatedMeshSceneNode*>(node)->isReadOnlyMaterials() );

	return !useMeshMaterial;
}



CColladaMeshWriterNames::CColladaMeshWriterNames(IColladaMeshWriter * writer)
	: ColladaMeshWriter(writer)
{
}

irr::core::stringw CColladaMeshWriterNames::nameForMesh(const scene::IMesh* mesh, int instance)
{
	irr::core::stringw name(L"mesh");
	name += nameForPtr(mesh);
	if ( instance > 0 )
	{
		name += L"i";
		name += irr::core::stringw(instance);
	}
	return ColladaMeshWriter->toNCName(name);
}

irr::core::stringw CColladaMeshWriterNames::nameForNode(const scene::ISceneNode* node)
{
	irr::core::stringw name;
	// Prefix, because xs::ID can't start with a number, also nicer name
	if ( node && node->getType() == ESNT_LIGHT )
		name = L"light";
	else
		name = L"node";
	name += nameForPtr(node);
	if ( node )
	{
		name += irr::core::stringw(node->getName());
	}
	return ColladaMeshWriter->toNCName(name);
}

irr::core::stringw CColladaMeshWriterNames::nameForMaterial(const video::SMaterial & material, int materialId, const scene::IMesh* mesh, const scene::ISceneNode* node)
{
	core::stringw strMat(L"mat");

	bool nodeMaterial = ColladaMeshWriter->getProperties()->useNodeMaterial(node);
	if ( nodeMaterial )
	{
		strMat += L"node";
		strMat += nameForPtr(node);
		strMat += irr::core::stringw(node->getName());
	}
	strMat += L"mesh";
	strMat += nameForPtr(mesh);
	strMat += materialId;
	return ColladaMeshWriter->toNCName(strMat);
}

irr::core::stringw CColladaMeshWriterNames::nameForPtr(const void* ptr) const
{
	wchar_t buf[32];
	swprintf(buf, 32, L"%p", ptr);
	return irr::core::stringw(buf);
}



CColladaMeshWriter::CColladaMeshWriter(	ISceneManager * smgr, video::IVideoDriver* driver,
					io::IFileSystem* fs)
	: FileSystem(fs), VideoDriver(driver), Writer(0)
{

	#ifdef _DEBUG
	setDebugName("CColladaMeshWriter");
	#endif

	if (VideoDriver)
		VideoDriver->grab();

	if (FileSystem)
		FileSystem->grab();

	if ( smgr )
		setAmbientLight( smgr->getAmbientLight() );

	CColladaMeshWriterProperties * p = new CColladaMeshWriterProperties();
	setDefaultProperties(p);
	setProperties(p);
	p->drop();

	CColladaMeshWriterNames * nameGenerator = new CColladaMeshWriterNames(this);
	setDefaultNameGenerator(nameGenerator);
	setNameGenerator(nameGenerator);
	nameGenerator->drop();
}


CColladaMeshWriter::~CColladaMeshWriter()
{
	if (VideoDriver)
		VideoDriver->drop();

	if (FileSystem)
		FileSystem->drop();
}


void CColladaMeshWriter::reset()
{
	LibraryImages.clear();
	Meshes.clear();
	LightNodes.clear();
	CameraNodes.clear();
	MaterialsWritten.clear();
	EffectsWritten.clear();
	MaterialNameCache.clear();
}

//! Returns the type of the mesh writer
EMESH_WRITER_TYPE CColladaMeshWriter::getType() const
{
	return EMWT_COLLADA;
}

//! writes a scene starting with the given node
bool CColladaMeshWriter::writeScene(io::IWriteFile* file, scene::ISceneNode* root)
{
	if (!file || !root)
		return false;

	reset();

	Writer = FileSystem->createXMLWriter(file);

	if (!Writer)
	{
		os::Printer::log("Could not write file", file->getFileName());
		return false;
	}

	Directory = FileSystem->getFileDir(FileSystem->getAbsolutePath( file->getFileName() ));

	// make names for all nodes with exportable meshes
	makeMeshNames(root);

	os::Printer::log("Writing scene", file->getFileName());

	// write COLLADA header

	Writer->writeXMLHeader();

	Writer->writeElement(L"COLLADA", false,
		L"xmlns", L"http://www.collada.org/2005/11/COLLADASchema",
		L"version", L"1.4.1");
	Writer->writeLineBreak();

	// write asset data
	writeAsset();

	// write all materials
	Writer->writeElement(L"library_materials", false);
	Writer->writeLineBreak();
	writeNodeMaterials(root);
	Writer->writeClosingTag(L"library_materials");
	Writer->writeLineBreak();

	Writer->writeElement(L"library_effects", false);
	Writer->writeLineBreak();
	writeNodeEffects(root);
	Writer->writeClosingTag(L"library_effects");
	Writer->writeLineBreak();


	// images
	writeLibraryImages();

	// lights
	Writer->writeElement(L"library_lights", false);
	Writer->writeLineBreak();

	writeAmbientLightElement( getAmbientLight() );
	writeNodeLights(root);

	Writer->writeClosingTag(L"library_lights");
	Writer->writeLineBreak();

	// cameras
	Writer->writeElement(L"library_cameras", false);
	Writer->writeLineBreak();
	writeNodeCameras(root);
	Writer->writeClosingTag(L"library_cameras");
	Writer->writeLineBreak();

	// write meshes
	Writer->writeElement(L"library_geometries", false);
	Writer->writeLineBreak();
	writeAllMeshGeometries();
	Writer->writeClosingTag(L"library_geometries");
	Writer->writeLineBreak();

	// write scene
	Writer->writeElement(L"library_visual_scenes", false);
	Writer->writeLineBreak();
	Writer->writeElement(L"visual_scene", false, L"id", L"default_scene");
	Writer->writeLineBreak();

		// ambient light (instance_light also needs a node as parent so we have to create one)
		Writer->writeElement(L"node", false);
		Writer->writeLineBreak();
		Writer->writeElement(L"instance_light", true, L"url", L"#ambientlight");
		Writer->writeLineBreak();
		Writer->writeClosingTag(L"node");
		Writer->writeLineBreak();

		// Write the scenegraph.
		if ( root->getType() != ESNT_SCENE_MANAGER )
		{
			// TODO: Not certain if we should really write the root or if we should just always only write the children.
			// For now writing root to keep backward compatibility for this case, but if anyone needs to _not_ write
			// that root-node we can add a parameter for this later on in writeScene.
			writeSceneNode(root);
		}
		else
		{
			// The visual_scene element is identical to our scenemanager and acts as root,
			// so we do not write the root itself if it points to the scenemanager.
			const core::list<ISceneNode*>& rootChildren = root->getChildren();
			for ( core::list<ISceneNode*>::ConstIterator it = rootChildren.begin();
					it != rootChildren.end();
					++ it )
			{
				writeSceneNode(*it);
			}
		}


	Writer->writeClosingTag(L"visual_scene");
	Writer->writeLineBreak();
	Writer->writeClosingTag(L"library_visual_scenes");
	Writer->writeLineBreak();


	// instance scene
	Writer->writeElement(L"scene", false);
	Writer->writeLineBreak();

		Writer->writeElement(L"instance_visual_scene", true, L"url", L"#default_scene");
		Writer->writeLineBreak();

	Writer->writeClosingTag(L"scene");
	Writer->writeLineBreak();


	// close everything

	Writer->writeClosingTag(L"COLLADA");
	Writer->drop();

	return true;
}

void CColladaMeshWriter::makeMeshNames(irr::scene::ISceneNode * node)
{
	if ( !node || !getProperties() || !getProperties()->isExportable(node) || !getNameGenerator())
		return;

	IMesh* mesh = getProperties()->getMesh(node);
	if ( mesh )
	{
		if ( !Meshes.find(mesh) )
		{
			SColladaMesh cm;
			cm.Name = nameForMesh(mesh, 0);
			Meshes.insert(mesh, cm);
		}
	}

	const core::list<ISceneNode*>& children = node->getChildren();
	for ( core::list<ISceneNode*>::ConstIterator it = children.begin(); it != children.end(); ++it )
	{
		makeMeshNames(*it);
	}
}

void CColladaMeshWriter::writeNodeMaterials(irr::scene::ISceneNode * node)
{
	if ( !node || !getProperties() || !getProperties()->isExportable(node) )
		return;

	core::array<irr::core::stringw> materialNames;

	IMesh* mesh = getProperties()->getMesh(node);
	if ( mesh )
	{
		MeshNode * n = Meshes.find(mesh);
		if ( !getProperties()->useNodeMaterial(node) )
		{
			// no material overrides - write mesh materials
			if ( n && !n->getValue().MaterialsWritten )
			{
				writeMeshMaterials(mesh, getGeometryWriting() == ECGI_PER_MESH_AND_MATERIAL ? &materialNames : NULL);
				n->getValue().MaterialsWritten = true;
			}
		}
		else
		{
			// write node materials
			for (u32 i=0; i<node->getMaterialCount(); ++i)
			{
				video::SMaterial & material = node->getMaterial(i);
				core::stringw strMat(nameForMaterial(material, i, mesh, node));
				writeMaterial(strMat);
				if ( getGeometryWriting() == ECGI_PER_MESH_AND_MATERIAL )
					materialNames.push_back(strMat);
			}
		}

		// When we write another mesh-geometry for each new material-list we have
		// to figure out here if we need another geometry copy and create a new name here.
		if ( n && getGeometryWriting() == ECGI_PER_MESH_AND_MATERIAL )
		{
			SGeometryMeshMaterials * geomMat = n->getValue().findGeometryMeshMaterials(materialNames);
			if ( geomMat )
				geomMat->MaterialOwners.push_back(node);
			else
			{
				SGeometryMeshMaterials gmm;
				if ( n->getValue().GeometryMeshMaterials.empty() )
					gmm.GeometryName = n->getValue().Name;	// first one can use the original name
				else
					gmm.GeometryName = 	nameForMesh(mesh, n->getValue().GeometryMeshMaterials.size());
				gmm.MaterialNames = materialNames;
				gmm.MaterialOwners.push_back(node);
				n->getValue().GeometryMeshMaterials.push_back(gmm);
			}
		}
	}

	const core::list<ISceneNode*>& children = node->getChildren();
	for ( core::list<ISceneNode*>::ConstIterator it = children.begin(); it != children.end(); ++it )
	{
		writeNodeMaterials( *it );
	}
}

void CColladaMeshWriter::writeMaterial(const irr::core::stringw& materialname)
{
	if ( MaterialsWritten.find(materialname) )
		return;
	MaterialsWritten.insert(materialname, true);

	Writer->writeElement(L"material", false,
		L"id", materialname.c_str(),
		L"name", materialname.c_str());
	Writer->writeLineBreak();

	// We don't make a difference between material and effect on export.
	// Every material is just using an instance of an effect.
	core::stringw strFx(materialname);
	strFx += L"-fx";
	Writer->writeElement(L"instance_effect", true,
		L"url", (core::stringw(L"#") + strFx).c_str());
	Writer->writeLineBreak();

	Writer->writeClosingTag(L"material");
	Writer->writeLineBreak();
}

void CColladaMeshWriter::writeNodeEffects(irr::scene::ISceneNode * node)
{
	if ( !node || !getProperties() || !getProperties()->isExportable(node) || !getNameGenerator() )
		return;

	IMesh* mesh = getProperties()->getMesh(node);
	if ( mesh )
	{
		if ( !getProperties()->useNodeMaterial(node) )
		{
			// no material overrides - write mesh materials
			MeshNode * n = Meshes.find(mesh);
			if ( n  && !n->getValue().EffectsWritten )
			{
				writeMeshEffects(mesh);
				n->getValue().EffectsWritten = true;
			}
		}
		else
		{
			// write node materials
			for (u32 i=0; i<node->getMaterialCount(); ++i)
			{
				video::SMaterial & material = node->getMaterial(i);
				irr::core::stringw materialfxname(nameForMaterial(material, i, mesh, node));
				materialfxname += L"-fx";
				writeMaterialEffect(materialfxname, material);
			}
		}
	}

	const core::list<ISceneNode*>& children = node->getChildren();
	for ( core::list<ISceneNode*>::ConstIterator it = children.begin(); it != children.end(); ++it )
	{
		writeNodeEffects( *it );
	}
}

void CColladaMeshWriter::writeNodeLights(irr::scene::ISceneNode * node)
{
	if ( !node || !getProperties() || !getProperties()->isExportable(node))
		return;

	if ( node->getType() == ESNT_LIGHT )
	{
		ILightSceneNode * lightNode = static_cast<ILightSceneNode*>(node);
		const video::SLight& lightData = lightNode->getLightData();

		SColladaLight cLight;
		cLight.Name = nameForNode(node);
		LightNodes.insert(node, cLight);

		Writer->writeElement(L"light", false, L"id", cLight.Name.c_str());
		Writer->writeLineBreak();

		Writer->writeElement(L"technique_common", false);
		Writer->writeLineBreak();

		switch ( lightNode->getLightType() )
		{
			case video::ELT_POINT:
				Writer->writeElement(L"point", false);
				Writer->writeLineBreak();

				writeColorElement(lightData.DiffuseColor, false);
				writeNode(L"constant_attenuation ", core::stringw(lightData.Attenuation.X).c_str());
				writeNode(L"linear_attenuation  ", core::stringw(lightData.Attenuation.Y).c_str());
				writeNode(L"quadratic_attenuation", core::stringw(lightData.Attenuation.Z).c_str());

				Writer->writeClosingTag(L"point");
				Writer->writeLineBreak();
				break;

			case video::ELT_SPOT:
				Writer->writeElement(L"spot", false);
				Writer->writeLineBreak();

				writeColorElement(lightData.DiffuseColor, false);

				writeNode(L"constant_attenuation ", core::stringw(lightData.Attenuation.X).c_str());
				writeNode(L"linear_attenuation  ", core::stringw(lightData.Attenuation.Y).c_str());
				writeNode(L"quadratic_attenuation", core::stringw(lightData.Attenuation.Z).c_str());

				writeNode(L"falloff_angle", core::stringw(lightData.OuterCone * core::RADTODEG).c_str());
				writeNode(L"falloff_exponent", core::stringw(lightData.Falloff).c_str());

				Writer->writeClosingTag(L"spot");
				Writer->writeLineBreak();
				break;

			case video::ELT_DIRECTIONAL:
				Writer->writeElement(L"directional", false);
				Writer->writeLineBreak();

				writeColorElement(lightData.DiffuseColor, false);

				Writer->writeClosingTag(L"directional");
				Writer->writeLineBreak();
				break;
			default:
				break;
		}

		Writer->writeClosingTag(L"technique_common");
		Writer->writeLineBreak();

		Writer->writeClosingTag(L"light");
		Writer->writeLineBreak();

	}

	const core::list<ISceneNode*>& children = node->getChildren();
	for ( core::list<ISceneNode*>::ConstIterator it = children.begin(); it != children.end(); ++it )
	{
		writeNodeLights( *it );
	}
}

void CColladaMeshWriter::writeNodeCameras(irr::scene::ISceneNode * node)
{
	if ( !node || !getProperties() || !getProperties()->isExportable(node) )
		return;

	if ( isCamera(node) )
	{
		ICameraSceneNode * cameraNode = static_cast<ICameraSceneNode*>(node);
		irr::core::stringw name = nameForNode(node);
		CameraNodes.insert(cameraNode, name);

		Writer->writeElement(L"camera", false, L"id", name.c_str());
		Writer->writeLineBreak();

		Writer->writeElement(L"optics", false);
		Writer->writeLineBreak();

		Writer->writeElement(L"technique_common", false);
		Writer->writeLineBreak();

		if ( cameraNode->isOrthogonal() )
		{
			Writer->writeElement(L"orthographic", false);
			Writer->writeLineBreak();

//			writeNode(L"xmag", core::stringw("1.0").c_str());	// TODO: do we need xmag, ymag?
//			writeNode(L"ymag", core::stringw("1.0").c_str());
			writeNode(L"aspect_ratio", core::stringw(cameraNode->getAspectRatio()).c_str());
			writeNode(L"znear", core::stringw(cameraNode->getNearValue()).c_str());
			writeNode(L"zfar", core::stringw(cameraNode->getFarValue()).c_str());

			Writer->writeClosingTag(L"orthographic");
			Writer->writeLineBreak();
		}
		else
		{
			Writer->writeElement(L"perspective", false);
			Writer->writeLineBreak();

			writeNode(L"yfov", core::stringw(cameraNode->getFOV()*core::RADTODEG).c_str());
			writeNode(L"aspect_ratio", core::stringw(cameraNode->getAspectRatio()).c_str());
			writeNode(L"znear", core::stringw(cameraNode->getNearValue()).c_str());
			writeNode(L"zfar", core::stringw(cameraNode->getFarValue()).c_str());

			Writer->writeClosingTag(L"perspective");
			Writer->writeLineBreak();
		}

		Writer->writeClosingTag(L"technique_common");
		Writer->writeLineBreak();

		Writer->writeClosingTag(L"optics");
		Writer->writeLineBreak();

		Writer->writeClosingTag(L"camera");
		Writer->writeLineBreak();
	}

	const core::list<ISceneNode*>& children = node->getChildren();
	for ( core::list<ISceneNode*>::ConstIterator it = children.begin(); it != children.end(); ++it )
	{
		writeNodeCameras( *it );
	}
}

void CColladaMeshWriter::writeAllMeshGeometries()
{
	core::map<IMesh*, SColladaMesh>::ConstIterator it = Meshes.getConstIterator();
	for(; !it.atEnd(); it++ )
	{
		IMesh* mesh = it->getKey();
		const SColladaMesh& colladaMesh = it->getValue();

		if ( getGeometryWriting() == ECGI_PER_MESH_AND_MATERIAL && colladaMesh.GeometryMeshMaterials.size() > 1 )
		{
			for ( u32 i=0; i<colladaMesh.GeometryMeshMaterials.size(); ++i )
			{
				writeMeshGeometry(colladaMesh.GeometryMeshMaterials[i].GeometryName, mesh);
			}
		}
		else
		{
			writeMeshGeometry(colladaMesh.Name, mesh);
		}
	}
}

void CColladaMeshWriter::writeSceneNode(irr::scene::ISceneNode * node )
{
	if ( !node || !getProperties() || !getProperties()->isExportable(node) )
		return;

	// Collada doesn't require to set the id, but some other tools have problems if none exists, so we just add it.
	irr::core::stringw nameId(nameForNode(node));
	Writer->writeElement(L"node", false, L"id", nameId.c_str());
	Writer->writeLineBreak();

	// DummyTransformationSceneNode don't have rotation, position, scale information
	// But also don't always export the transformation matrix as that forces us creating
	// new DummyTransformationSceneNode's on import.
	if ( node->getType() == ESNT_DUMMY_TRANSFORMATION )
	{
		writeMatrixElement(node->getRelativeTransformation());
	}
	else
	{
		irr::core::vector3df rot(node->getRotation());
		if ( isCamera(node) && !static_cast<ICameraSceneNode*>(node)->getTargetAndRotationBinding() )
		{
			ICameraSceneNode * camNode = static_cast<ICameraSceneNode*>(node);
			const core::vector3df toTarget = camNode->getTarget() - camNode->getAbsolutePosition();
			rot = toTarget.getHorizontalAngle();
		}

		writeTranslateElement( node->getPosition() );
		writeRotateElement( irr::core::vector3df(1.f, 0.f, 0.f), rot.X );
		writeRotateElement( irr::core::vector3df(0.f, 1.f, 0.f), rot.Y );
		writeRotateElement( irr::core::vector3df(0.f, 0.f, 1.f), rot.Z );
		writeScaleElement( node->getScale() );
	}

	// instance geometry
	IMesh* mesh = getProperties()->getMesh(node);
	if ( mesh )
	{
		MeshNode * n = Meshes.find(mesh);
		if ( n )
		{
			const SColladaMesh& colladaMesh = n->getValue();
			writeMeshInstanceGeometry(colladaMesh.findGeometryNameForNode(node), mesh, node);
		}
	}

	// instance light
	if ( node->getType() == ESNT_LIGHT )
	{
		LightNode * n = LightNodes.find(node);
		if ( n )
			writeLightInstance(n->getValue().Name);
	}

	// instance camera
	if ( isCamera(node) )
	{
		CameraNode * camNode = CameraNodes.find(node);
		if ( camNode )
			writeCameraInstance(camNode->getValue());
	}

	const core::list<ISceneNode*>& children = node->getChildren();
	for ( core::list<ISceneNode*>::ConstIterator it = children.begin(); it != children.end(); ++it )
	{
		writeSceneNode( *it );
	}

	Writer->writeClosingTag(L"node");
	Writer->writeLineBreak();
}

//! writes a mesh
bool CColladaMeshWriter::writeMesh(io::IWriteFile* file, scene::IMesh* mesh, s32 flags)
{
	if (!file)
		return false;

	reset();

	Writer = FileSystem->createXMLWriter(file);

	if (!Writer)
	{
		os::Printer::log("Could not write file", file->getFileName());
		return false;
	}

	Directory = FileSystem->getFileDir(FileSystem->getAbsolutePath( file->getFileName() ));

	os::Printer::log("Writing mesh", file->getFileName());

	// write COLLADA header

	Writer->writeXMLHeader();

	Writer->writeElement(L"COLLADA", false,
		L"xmlns", L"http://www.collada.org/2005/11/COLLADASchema",
		L"version", L"1.4.1");
	Writer->writeLineBreak();

	// write asset data
	writeAsset();

	// write all materials

	Writer->writeElement(L"library_materials", false);
	Writer->writeLineBreak();

	writeMeshMaterials(mesh);

	Writer->writeClosingTag(L"library_materials");
	Writer->writeLineBreak();

	Writer->writeElement(L"library_effects", false);
	Writer->writeLineBreak();

	writeMeshEffects(mesh);

	Writer->writeClosingTag(L"library_effects");
	Writer->writeLineBreak();

	// images
	writeLibraryImages();

	// write mesh

	Writer->writeElement(L"library_geometries", false);
	Writer->writeLineBreak();

	irr::core::stringw meshname(nameForMesh(mesh, 0));
	writeMeshGeometry(meshname, mesh);

	Writer->writeClosingTag(L"library_geometries");
	Writer->writeLineBreak();

	// write scene_library
	if ( getWriteDefaultScene() )
	{
		Writer->writeElement(L"library_visual_scenes", false);
		Writer->writeLineBreak();

		Writer->writeElement(L"visual_scene", false, L"id", L"default_scene");
		Writer->writeLineBreak();

			Writer->writeElement(L"node", false);
			Writer->writeLineBreak();

				writeMeshInstanceGeometry(meshname, mesh);

			Writer->writeClosingTag(L"node");
			Writer->writeLineBreak();

		Writer->writeClosingTag(L"visual_scene");
		Writer->writeLineBreak();

		Writer->writeClosingTag(L"library_visual_scenes");
		Writer->writeLineBreak();


		// write scene
		Writer->writeElement(L"scene", false);
		Writer->writeLineBreak();

			Writer->writeElement(L"instance_visual_scene", true, L"url", L"#default_scene");
			Writer->writeLineBreak();

		Writer->writeClosingTag(L"scene");
		Writer->writeLineBreak();
	}


	// close everything

	Writer->writeClosingTag(L"COLLADA");
	Writer->drop();

	return true;
}

void CColladaMeshWriter::writeMeshInstanceGeometry(const irr::core::stringw& meshname, scene::IMesh* mesh, scene::ISceneNode* node)
{
	//<instance_geometry url="#mesh">
	Writer->writeElement(L"instance_geometry", false, L"url", toRef(meshname).c_str());
	Writer->writeLineBreak();

		Writer->writeElement(L"bind_material", false);
		Writer->writeLineBreak();

			Writer->writeElement(L"technique_common", false);
			Writer->writeLineBreak();

			// instance materials
			// <instance_material symbol="leaf" target="#MidsummerLeaf01"/>
			bool useNodeMaterials = node && node->getMaterialCount() == mesh->getMeshBufferCount();
			for (u32 i=0; i<mesh->getMeshBufferCount(); ++i)
			{
				irr::core::stringw strMatSymbol(nameForMaterialSymbol(mesh, i));
				core::stringw strMatTarget = "#";
				video::SMaterial & material = useNodeMaterials ? node->getMaterial(i) : mesh->getMeshBuffer(i)->getMaterial();
				strMatTarget += nameForMaterial(material, i, mesh, node);
				Writer->writeElement(L"instance_material", false, L"symbol", strMatSymbol.c_str(), L"target", strMatTarget.c_str());
				Writer->writeLineBreak();

					// TODO: need to handle second UV-set
					// <bind_vertex_input semantic="uv" input_semantic="TEXCOORD" input_set="0"/>
					Writer->writeElement(L"bind_vertex_input", true, L"semantic", L"uv", L"input_semantic", L"TEXCOORD", L"input_set", L"0" );
					Writer->writeLineBreak();

				Writer->writeClosingTag(L"instance_material");
				Writer->writeLineBreak();
			}

			Writer->writeClosingTag(L"technique_common");
			Writer->writeLineBreak();

		Writer->writeClosingTag(L"bind_material");
		Writer->writeLineBreak();

	Writer->writeClosingTag(L"instance_geometry");
	Writer->writeLineBreak();
}

void CColladaMeshWriter::writeLightInstance(const irr::core::stringw& lightName)
{
	Writer->writeElement(L"instance_light", true, L"url", toRef(lightName).c_str());
	Writer->writeLineBreak();
}

void CColladaMeshWriter::writeCameraInstance(const irr::core::stringw& cameraName)
{
	Writer->writeElement(L"instance_camera", true, L"url", toRef(cameraName).c_str());
	Writer->writeLineBreak();
}

bool CColladaMeshWriter::hasSecondTextureCoordinates(video::E_VERTEX_TYPE type) const
{
	return type == video::EVT_2TCOORDS;
}

void CColladaMeshWriter::writeVector(const irr::core::vector3df& vec)
{
	wchar_t tmpbuf[255];
	swprintf(tmpbuf, 255, L"%f %f %f", vec.X, vec.Y, vec.Z);

	Writer->writeText(tmpbuf);
}

void CColladaMeshWriter::writeUv(const irr::core::vector2df& vec)
{
	// change handedness
	wchar_t tmpbuf[255];
	swprintf(tmpbuf, 255, L"%f %f", vec.X, 1.f-vec.Y);

	Writer->writeText(tmpbuf);
}

void CColladaMeshWriter::writeVector(const irr::core::vector2df& vec)
{
	wchar_t tmpbuf[255];
	swprintf(tmpbuf, 255, L"%f %f", vec.X, vec.Y);

	Writer->writeText(tmpbuf);
}

void CColladaMeshWriter::writeColor(const irr::video::SColorf& colorf, bool writeAlpha)
{
	wchar_t tmpbuf[255];
	if ( writeAlpha )
		swprintf(tmpbuf, 255, L"%f %f %f %f", colorf.getRed(), colorf.getGreen(), colorf.getBlue(), colorf.getAlpha());
	else
		swprintf(tmpbuf, 255, L"%f %f %f", colorf.getRed(), colorf.getGreen(), colorf.getBlue());

	Writer->writeText(tmpbuf);
}

irr::core::stringw CColladaMeshWriter::toString(const irr::video::ECOLOR_FORMAT format) const
{
	switch ( format )
	{
		case video::ECF_A1R5G5B5:	return irr::core::stringw(L"A1R5G5B5");
		case video::ECF_R5G6B5:		return irr::core::stringw(L"R5G6B5");
		case video::ECF_R8G8B8:		return irr::core::stringw(L"R8G8B8");
		case video::ECF_A8R8G8B8:	return irr::core::stringw(L"A8R8G8B8");
		default:					return irr::core::stringw(L"");
	}
}

irr::core::stringw CColladaMeshWriter::toString(const irr::video::E_TEXTURE_CLAMP clamp) const
{
	switch ( clamp )
	{
		case video::ETC_REPEAT:
			return core::stringw(L"WRAP");
		case video::ETC_CLAMP:
		case video::ETC_CLAMP_TO_EDGE:
			return core::stringw(L"CLAMP");
		case video::ETC_CLAMP_TO_BORDER:
			return core::stringw(L"BORDER");
		case video::ETC_MIRROR:
		case video::ETC_MIRROR_CLAMP:
		case video::ETC_MIRROR_CLAMP_TO_EDGE:
		case video::ETC_MIRROR_CLAMP_TO_BORDER:
			return core::stringw(L"MIRROR");
	}
	return core::stringw(L"NONE");
}

irr::core::stringw CColladaMeshWriter::toString(const irr::scene::E_COLLADA_TRANSPARENT_FX transparent) const
{
	if ( transparent & ECOF_RGB_ZERO )
		return core::stringw(L"RGB_ZERO");
	else
		return core::stringw(L"A_ONE");
}

irr::core::stringw CColladaMeshWriter::toRef(const irr::core::stringw& source) const
{
	irr::core::stringw ref(L"#");
	ref += source;
	return ref;
}

bool CColladaMeshWriter::isCamera(const scene::ISceneNode* node) const
{
	// TODO: we need some ISceneNode::hasType() function to get rid of those checks
	if (	node->getType() == ESNT_CAMERA
		||	node->getType() == ESNT_CAMERA_MAYA
		||	node->getType() == ESNT_CAMERA_FPS )
		return true;
	return false;
}

irr::core::stringw CColladaMeshWriter::nameForMesh(const scene::IMesh* mesh, int instance) const
{
	IColladaMeshWriterNames * nameGenerator = getNameGenerator();
	if ( nameGenerator )
	{
		return nameGenerator->nameForMesh(mesh, instance);
	}
	return irr::core::stringw(L"missing_name_generator");
}

irr::core::stringw CColladaMeshWriter::nameForNode(const scene::ISceneNode* node) const
{
	IColladaMeshWriterNames * nameGenerator = getNameGenerator();
	if ( nameGenerator )
	{
		return nameGenerator->nameForNode(node);
	}
	return irr::core::stringw(L"missing_name_generator");
}

irr::core::stringw CColladaMeshWriter::nameForMaterial(const video::SMaterial & material, int materialId, const scene::IMesh* mesh, const scene::ISceneNode* node)
{
	irr::core::stringw matName;
	if ( getExportSMaterialsOnlyOnce() )
	{
		matName = findCachedMaterialName(material);
		if ( !matName.empty() )
			return matName;
	}

	IColladaMeshWriterNames * nameGenerator = getNameGenerator();
	if ( nameGenerator )
	{
		matName = nameGenerator->nameForMaterial(material, materialId, mesh, node);
	}
	else
		matName = irr::core::stringw(L"missing_name_generator");

	if ( getExportSMaterialsOnlyOnce() )
		MaterialNameCache.push_back (MaterialName(material, matName));
	return matName;
}

// Each mesh-material has one symbol which is replaced on instantiation
irr::core::stringw CColladaMeshWriter::nameForMaterialSymbol(const scene::IMesh* mesh, int materialId) const
{
	wchar_t buf[100];
	swprintf(buf, 100, L"mat_symb_%p_%d", mesh, materialId);
	return irr::core::stringw(buf);
}

irr::core::stringw CColladaMeshWriter::findCachedMaterialName(const irr::video::SMaterial& material) const
{
	for ( u32 i=0; i<MaterialNameCache.size(); ++i )
	{
		if ( MaterialNameCache[i].Material == material )
			return MaterialNameCache[i].Name;
	}
	return irr::core::stringw();
}

irr::core::stringw CColladaMeshWriter::minTexfilterToString(bool bilinear, bool trilinear) const
{
	if ( trilinear )
		return core::stringw(L"LINEAR_MIPMAP_LINEAR");
	else if ( bilinear )
		return core::stringw(L"LINEAR_MIPMAP_NEAREST");

	return core::stringw(L"NONE");
}

inline irr::core::stringw CColladaMeshWriter::magTexfilterToString(bool bilinear, bool trilinear) const
{
	if ( bilinear || trilinear )
		return core::stringw(L"LINEAR");

	return core::stringw(L"NONE");
}

bool CColladaMeshWriter::isXmlNameStartChar(wchar_t c) const
{
	return		(c >= 'A' && c <= 'Z')
			||	c == L'_'
			||	(c >= 'a' && c <= 'z')
			||	(c >= 0xC0 && c <= 0xD6)
			||	(c >= 0xD8 && c <= 0xF6)
			||	(c >= 0xF8 && c <= 0x2FF)
			||  (c >= 0x370 && c <= 0x37D)
			||  (c >= 0x37F && c <= 0x1FFF)
			||  (c >= 0x200C && c <= 0x200D)
			||  (c >= 0x2070 && c <= 0x218F)
			||  (c >= 0x2C00 && c <= 0x2FEF)
			||  (c >= 0x3001 && c <= 0xD7FF)
			||  (c >= 0xF900 && c <= 0xFDCF)
			||  (c >= 0xFDF0 && c <= 0xFFFD)
#if __SIZEOF_WCHAR_T__ == 4 || __WCHAR_MAX__ > 0x10000
			||  (c >= 0x10000 && c <= 0xEFFFF)
#endif
			;
}

bool CColladaMeshWriter::isXmlNameChar(wchar_t c) const
{
	return isXmlNameStartChar(c)
		||	c == L'-'
		||	c == L'.'
		||	(c >= '0' && c <= '9')
		||	c == 0xB7
		||	(c >= 0x0300 && c <= 0x036F)
		||	(c >= 0x203F && c <= 0x2040);
}

// Restrict the characters to a set of allowed characters in xs::NCName.
irr::core::stringw CColladaMeshWriter::toNCName(const irr::core::stringw& oldString, const irr::core::stringw& prefix) const
{
	irr::core::stringw result(prefix);	// help to ensure id starts with a valid char and reduce chance of name-conflicts
	if ( oldString.empty() )
		return result;

	result.append( oldString );

	// We replace all characters not allowed by a replacement char
	const wchar_t REPLACMENT = L'-';
	for ( irr::u32 i=1; i < result.size(); ++i )
	{
		if ( result[i] == L':' || !isXmlNameChar(result[i]) )
		{
			result[i] = REPLACMENT;
		}
	}
	return result;
}

// Restrict the characters to a set of allowed characters in xs::NCName.
irr::core::stringw CColladaMeshWriter::pathToURI(const irr::io::path& path) const
{
	irr::core::stringw result;

	// is this a relative path?
	if ( path.size() > 1
		&& path[0] != _IRR_TEXT('/')
		&& path[0] != _IRR_TEXT('\\')
		&& path[1] != _IRR_TEXT(':') )
	{
		// not already starting with "./" ?
		if (	path[0] != _IRR_TEXT('.')
			||	path[1] != _IRR_TEXT('/') )
		{
			result.append(L"./");
		}
	}
	result.append(path);

	// TODO: make correct URI (without whitespaces)

	return result;
}

void CColladaMeshWriter::writeAsset()
{
	Writer->writeElement(L"asset", false);
	Writer->writeLineBreak();

	Writer->writeElement(L"contributor", false);
	Writer->writeLineBreak();
	Writer->writeElement(L"authoring_tool", false);
	Writer->writeText(L"Irrlicht Engine / irrEdit");  // this code has originated from irrEdit 0.7
	Writer->writeClosingTag(L"authoring_tool");
	Writer->writeLineBreak();
	Writer->writeClosingTag(L"contributor");
	Writer->writeLineBreak();

	// The next two are required
	Writer->writeElement(L"created", false);
	Writer->writeText(L"2008-01-31T00:00:00Z");
	Writer->writeClosingTag(L"created");
	Writer->writeLineBreak();

	Writer->writeElement(L"modified", false);
	Writer->writeText(L"2008-01-31T00:00:00Z");
	Writer->writeClosingTag(L"modified");
	Writer->writeLineBreak();

	Writer->writeElement(L"revision", false);
	Writer->writeText(L"1.0");
	Writer->writeClosingTag(L"revision");
	Writer->writeLineBreak();

	Writer->writeClosingTag(L"asset");
	Writer->writeLineBreak();
}

void CColladaMeshWriter::writeMeshMaterials(scene::IMesh* mesh, irr::core::array<irr::core::stringw> * materialNamesOut)
{
	u32 i;
	for (i=0; i<mesh->getMeshBufferCount(); ++i)
	{
		video::SMaterial & material = mesh->getMeshBuffer(i)->getMaterial();
		core::stringw strMat(nameForMaterial(material, i, mesh, NULL));
		writeMaterial(strMat);
		if ( materialNamesOut )
			materialNamesOut->push_back(strMat);
	}
}

void CColladaMeshWriter::writeMaterialEffect(const irr::core::stringw& materialfxname, const video::SMaterial & material)
{
	if ( EffectsWritten.find(materialfxname) )
		return;
	EffectsWritten.insert(materialfxname, true);

	Writer->writeElement(L"effect", false,
		L"id", materialfxname.c_str(),
		L"name", materialfxname.c_str());
	Writer->writeLineBreak();
	Writer->writeElement(L"profile_COMMON", false);
	Writer->writeLineBreak();

	int numTextures = 0;
	if ( getWriteTextures() )
	{
		// write texture surfaces and samplers and buffer all used imagess
		for ( int t=0; t<4; ++t )
		{
			const video::SMaterialLayer& layer  = material.TextureLayer[t];
			if ( !layer.Texture )
				break;
			++numTextures;

			if ( LibraryImages.linear_search(layer.Texture) < 0 )
					LibraryImages.push_back( layer.Texture );

			irr::core::stringw texName("tex");
			texName += irr::core::stringw(t);

			// write texture surface
			//<newparam sid="tex0-surface">
			irr::core::stringw texSurface(texName);
			texSurface += L"-surface";
			Writer->writeElement(L"newparam", false, L"sid", texSurface.c_str());
			Writer->writeLineBreak();
			//  <surface type="2D">
				Writer->writeElement(L"surface", false, L"type", L"2D");
				Writer->writeLineBreak();

		//          <init_from>internal_texturename</init_from>
					Writer->writeElement(L"init_from", false);
					irr::io::path p(FileSystem->getRelativeFilename(layer.Texture->getName().getPath(), Directory));
					Writer->writeText(toNCName(irr::core::stringw(p)).c_str());
					Writer->writeClosingTag(L"init_from");
					Writer->writeLineBreak();

		//          <format>A8R8G8B8</format>
					Writer->writeElement(L"format", false);
					video::ECOLOR_FORMAT format = layer.Texture->getColorFormat();
					Writer->writeText(toString(format).c_str());
					Writer->writeClosingTag(L"format");
					Writer->writeLineBreak();
		//      </surface>
				Writer->writeClosingTag(L"surface");
				Writer->writeLineBreak();
		//  </newparam>
			Writer->writeClosingTag(L"newparam");
			Writer->writeLineBreak();

			// write texture sampler
		//  <newparam sid="tex0-sampler">
			irr::core::stringw texSampler(texName);
			texSampler += L"-sampler";
			Writer->writeElement(L"newparam", false, L"sid", texSampler.c_str());
			Writer->writeLineBreak();
		//      <sampler2D>
				Writer->writeElement(L"sampler2D", false);
				Writer->writeLineBreak();

		//          <source>tex0-surface</source>
					Writer->writeElement(L"source", false);
					Writer->writeText(texSurface.c_str());
					Writer->writeClosingTag(L"source");
					Writer->writeLineBreak();

		//			<wrap_s>WRAP</wrap_s>
					Writer->writeElement(L"wrap_s", false);
					Writer->writeText(toString((video::E_TEXTURE_CLAMP)layer.TextureWrapU).c_str());
					Writer->writeClosingTag(L"wrap_s");
					Writer->writeLineBreak();

		//			<wrap_t>WRAP</wrap_t>
					Writer->writeElement(L"wrap_t", false);
					Writer->writeText(toString((video::E_TEXTURE_CLAMP)layer.TextureWrapV).c_str());
					Writer->writeClosingTag(L"wrap_t");
					Writer->writeLineBreak();

		//			<minfilter>LINEAR_MIPMAP_LINEAR</minfilter>
					Writer->writeElement(L"minfilter", false);
					Writer->writeText(minTexfilterToString(layer.BilinearFilter, layer.TrilinearFilter).c_str());
					Writer->writeClosingTag(L"minfilter");
					Writer->writeLineBreak();

		//			<magfilter>LINEAR</magfilter>
					Writer->writeElement(L"magfilter", false);
					Writer->writeText(magTexfilterToString(layer.BilinearFilter, layer.TrilinearFilter).c_str());
					Writer->writeClosingTag(L"magfilter");
					Writer->writeLineBreak();

					// TBD - actually not sure how anisotropic should be written, so for now it writes in a way
					// that works with the way the loader reads it again.
					if ( layer.AnisotropicFilter )
					{
		//			<mipfilter>LINEAR_MIPMAP_LINEAR</mipfilter>
						Writer->writeElement(L"mipfilter", false);
						Writer->writeText(L"LINEAR_MIPMAP_LINEAR");
						Writer->writeClosingTag(L"mipfilter");
						Writer->writeLineBreak();
					}

		//     </sampler2D>
				Writer->writeClosingTag(L"sampler2D");
				Writer->writeLineBreak();
		//  </newparam>
			Writer->writeClosingTag(L"newparam");
			Writer->writeLineBreak();
		}
	}

	Writer->writeElement(L"technique", false, L"sid", L"common");
	Writer->writeLineBreak();

	E_COLLADA_TECHNIQUE_FX techFx = getProperties() ? getProperties()->getTechniqueFx(material) : ECTF_BLINN;
	writeFxElement(material, techFx);

	Writer->writeClosingTag(L"technique");
	Writer->writeLineBreak();
	Writer->writeClosingTag(L"profile_COMMON");
	Writer->writeLineBreak();
	Writer->writeClosingTag(L"effect");
	Writer->writeLineBreak();
}

void CColladaMeshWriter::writeMeshEffects(scene::IMesh* mesh)
{
	for (u32 i=0; i<mesh->getMeshBufferCount(); ++i)
	{
		video::SMaterial & material = mesh->getMeshBuffer(i)->getMaterial();
		irr::core::stringw materialfxname(nameForMaterial(material, i, mesh, NULL));
		materialfxname += L"-fx";
		writeMaterialEffect(materialfxname, material);
	}
}

void CColladaMeshWriter::writeMeshGeometry(const irr::core::stringw& meshname, scene::IMesh* mesh)
{
	core::stringw meshId(meshname);

	Writer->writeElement(L"geometry", false, L"id", meshId.c_str(), L"name", meshId.c_str());
	Writer->writeLineBreak();
	Writer->writeElement(L"mesh");
	Writer->writeLineBreak();

	// do some statistics for the mesh to know which stuff needs to be saved into
	// the file:
	// - count vertices
	// - check for the need of a second texture coordinate
	// - count amount of second texture coordinates
	// - check for the need of tangents (TODO)

	u32 totalVertexCount = 0;
	u32 totalTCoords2Count = 0;
	bool needsTangents = false; // TODO: tangents not supported here yet
	u32 i=0;
	for (i=0; i<mesh->getMeshBufferCount(); ++i)
	{
		totalVertexCount += mesh->getMeshBuffer(i)->getVertexCount();

		if (hasSecondTextureCoordinates(mesh->getMeshBuffer(i)->getVertexType()))
			totalTCoords2Count += mesh->getMeshBuffer(i)->getVertexCount();

		if (!needsTangents)
			needsTangents = mesh->getMeshBuffer(i)->getVertexType() == video::EVT_TANGENTS;
	}

	SComponentGlobalStartPos* globalIndices = new SComponentGlobalStartPos[mesh->getMeshBufferCount()];

	// write positions
	core::stringw meshPosId(meshId);
	meshPosId += L"-Pos";
	Writer->writeElement(L"source", false, L"id", meshPosId.c_str());
	Writer->writeLineBreak();

		core::stringw vertexCountStr(totalVertexCount*3);
		core::stringw meshPosArrayId(meshPosId);
		meshPosArrayId += L"-array";
		Writer->writeElement(L"float_array", false, L"id", meshPosArrayId.c_str(),
					L"count", vertexCountStr.c_str());
		Writer->writeLineBreak();

		for (i=0; i<mesh->getMeshBufferCount(); ++i)
		{
			scene::IMeshBuffer* buffer = mesh->getMeshBuffer(i);
			video::E_VERTEX_TYPE vtxType = buffer->getVertexType();
			u32 vertexCount = buffer->getVertexCount();

			globalIndices[i].PosStartIndex = 0;

			if (i!=0)
				globalIndices[i].PosStartIndex = globalIndices[i-1].PosLastIndex + 1;

			globalIndices[i].PosLastIndex = globalIndices[i].PosStartIndex + vertexCount - 1;

			switch(vtxType)
			{
			case video::EVT_STANDARD:
				{
					video::S3DVertex* vtx = (video::S3DVertex*)buffer->getVertices();
					for (u32 j=0; j<vertexCount; ++j)
					{
						writeVector(vtx[j].Pos);
						Writer->writeLineBreak();
					}
				}
				break;
			case video::EVT_2TCOORDS:
				{
					video::S3DVertex2TCoords* vtx = (video::S3DVertex2TCoords*)buffer->getVertices();
					for (u32 j=0; j<vertexCount; ++j)
					{
						writeVector(vtx[j].Pos);
						Writer->writeLineBreak();
					}
				}
				break;
			case video::EVT_TANGENTS:
				{
					video::S3DVertexTangents* vtx = (video::S3DVertexTangents*)buffer->getVertices();
					for (u32 j=0; j<vertexCount; ++j)
					{
						writeVector(vtx[j].Pos);
						Writer->writeLineBreak();
					}
				}
				break;
			}
		}

		Writer->writeClosingTag(L"float_array");
		Writer->writeLineBreak();

		Writer->writeElement(L"technique_common", false);
		Writer->writeLineBreak();

		vertexCountStr = core::stringw(totalVertexCount);

			Writer->writeElement(L"accessor", false, L"source", toRef(meshPosArrayId).c_str(),
						L"count", vertexCountStr.c_str(), L"stride", L"3");
			Writer->writeLineBreak();

				Writer->writeElement(L"param", true, L"name", L"X", L"type", L"float");
				Writer->writeLineBreak();
				Writer->writeElement(L"param", true, L"name", L"Y", L"type", L"float");
				Writer->writeLineBreak();
				Writer->writeElement(L"param", true, L"name", L"Z", L"type", L"float");
				Writer->writeLineBreak();

				Writer->writeClosingTag(L"accessor");
				Writer->writeLineBreak();

		Writer->writeClosingTag(L"technique_common");
		Writer->writeLineBreak();

	Writer->writeClosingTag(L"source");
	Writer->writeLineBreak();

	// write texture coordinates

	core::stringw meshTexCoord0Id(meshId);
	meshTexCoord0Id += L"-TexCoord0";
	Writer->writeElement(L"source", false, L"id", meshTexCoord0Id.c_str());
	Writer->writeLineBreak();

		vertexCountStr = core::stringw(totalVertexCount*2);
		core::stringw meshTexCoordArrayId(meshTexCoord0Id);
		meshTexCoordArrayId += L"-array";
		Writer->writeElement(L"float_array", false, L"id", meshTexCoordArrayId.c_str(),
					L"count", vertexCountStr.c_str());
		Writer->writeLineBreak();

		for (i=0; i<mesh->getMeshBufferCount(); ++i)
		{
			scene::IMeshBuffer* buffer = mesh->getMeshBuffer(i);
			video::E_VERTEX_TYPE vtxType = buffer->getVertexType();
			u32 vertexCount = buffer->getVertexCount();

			globalIndices[i].TCoord0StartIndex = 0;

			if (i!=0)
				globalIndices[i].TCoord0StartIndex = globalIndices[i-1].TCoord0LastIndex + 1;

			globalIndices[i].TCoord0LastIndex = globalIndices[i].TCoord0StartIndex + vertexCount - 1;

			switch(vtxType)
			{
			case video::EVT_STANDARD:
				{
					video::S3DVertex* vtx = (video::S3DVertex*)buffer->getVertices();
					for (u32 j=0; j<vertexCount; ++j)
					{
						writeUv(vtx[j].TCoords);
						Writer->writeLineBreak();
					}
				}
				break;
			case video::EVT_2TCOORDS:
				{
					video::S3DVertex2TCoords* vtx = (video::S3DVertex2TCoords*)buffer->getVertices();
					for (u32 j=0; j<vertexCount; ++j)
					{
						writeUv(vtx[j].TCoords);
						Writer->writeLineBreak();
					}
				}
				break;
			case video::EVT_TANGENTS:
				{
					video::S3DVertexTangents* vtx = (video::S3DVertexTangents*)buffer->getVertices();
					for (u32 j=0; j<vertexCount; ++j)
					{
						writeUv(vtx[j].TCoords);
						Writer->writeLineBreak();
					}
				}
				break;
			}
		}

		Writer->writeClosingTag(L"float_array");
		Writer->writeLineBreak();

		Writer->writeElement(L"technique_common", false);
		Writer->writeLineBreak();

		vertexCountStr = core::stringw(totalVertexCount);

			Writer->writeElement(L"accessor", false, L"source", toRef(meshTexCoordArrayId).c_str(),
						L"count", vertexCountStr.c_str(), L"stride", L"2");
			Writer->writeLineBreak();

				Writer->writeElement(L"param", true, L"name", L"U", L"type", L"float");
				Writer->writeLineBreak();
				Writer->writeElement(L"param", true, L"name", L"V", L"type", L"float");
				Writer->writeLineBreak();

			Writer->writeClosingTag(L"accessor");
			Writer->writeLineBreak();

		Writer->writeClosingTag(L"technique_common");
		Writer->writeLineBreak();

	Writer->writeClosingTag(L"source");
	Writer->writeLineBreak();

	// write normals
	core::stringw meshNormalId(meshId);
	meshNormalId += L"-Normal";
	Writer->writeElement(L"source", false, L"id", meshNormalId.c_str());
	Writer->writeLineBreak();

		vertexCountStr = core::stringw(totalVertexCount*3);
		core::stringw meshNormalArrayId(meshNormalId);
		meshNormalArrayId += L"-array";
		Writer->writeElement(L"float_array", false, L"id", meshNormalArrayId.c_str(),
					L"count", vertexCountStr.c_str());
		Writer->writeLineBreak();

		for (i=0; i<mesh->getMeshBufferCount(); ++i)
		{
			scene::IMeshBuffer* buffer = mesh->getMeshBuffer(i);
			video::E_VERTEX_TYPE vtxType = buffer->getVertexType();
			u32 vertexCount = buffer->getVertexCount();

			globalIndices[i].NormalStartIndex = 0;

			if (i!=0)
				globalIndices[i].NormalStartIndex = globalIndices[i-1].NormalLastIndex + 1;

			globalIndices[i].NormalLastIndex = globalIndices[i].NormalStartIndex + vertexCount - 1;

			switch(vtxType)
			{
			case video::EVT_STANDARD:
				{
					video::S3DVertex* vtx = (video::S3DVertex*)buffer->getVertices();
					for (u32 j=0; j<vertexCount; ++j)
					{
						writeVector(vtx[j].Normal);
						Writer->writeLineBreak();
					}
				}
				break;
			case video::EVT_2TCOORDS:
				{
					video::S3DVertex2TCoords* vtx = (video::S3DVertex2TCoords*)buffer->getVertices();
					for (u32 j=0; j<vertexCount; ++j)
					{
						writeVector(vtx[j].Normal);
						Writer->writeLineBreak();
					}
				}
				break;
			case video::EVT_TANGENTS:
				{
					video::S3DVertexTangents* vtx = (video::S3DVertexTangents*)buffer->getVertices();
					for (u32 j=0; j<vertexCount; ++j)
					{
						writeVector(vtx[j].Normal);
						Writer->writeLineBreak();
					}
				}
				break;
			}
		}

		Writer->writeClosingTag(L"float_array");
		Writer->writeLineBreak();

		Writer->writeElement(L"technique_common", false);
		Writer->writeLineBreak();

		vertexCountStr = core::stringw(totalVertexCount);

		Writer->writeElement(L"accessor", false, L"source", toRef(meshNormalArrayId).c_str(),
									L"count", vertexCountStr.c_str(), L"stride", L"3");
			Writer->writeLineBreak();

				Writer->writeElement(L"param", true, L"name", L"X", L"type", L"float");
				Writer->writeLineBreak();
				Writer->writeElement(L"param", true, L"name", L"Y", L"type", L"float");
				Writer->writeLineBreak();
				Writer->writeElement(L"param", true, L"name", L"Z", L"type", L"float");
				Writer->writeLineBreak();

			Writer->writeClosingTag(L"accessor");
			Writer->writeLineBreak();

		Writer->writeClosingTag(L"technique_common");
		Writer->writeLineBreak();

	Writer->writeClosingTag(L"source");
	Writer->writeLineBreak();

	// write second set of texture coordinates
	core::stringw meshTexCoord1Id(meshId);
	meshTexCoord1Id += L"-TexCoord1";
	if (totalTCoords2Count)
	{
		Writer->writeElement(L"source", false, L"id", meshTexCoord1Id.c_str());
		Writer->writeLineBreak();

			vertexCountStr = core::stringw(totalTCoords2Count*2);
			core::stringw meshTexCoord1ArrayId(meshTexCoord1Id);
			meshTexCoord1ArrayId += L"-array";
			Writer->writeElement(L"float_array", false, L"id", meshTexCoord1ArrayId.c_str(),
									L"count", vertexCountStr.c_str());
			Writer->writeLineBreak();

			for (i=0; i<mesh->getMeshBufferCount(); ++i)
			{
				scene::IMeshBuffer* buffer = mesh->getMeshBuffer(i);
				video::E_VERTEX_TYPE vtxType = buffer->getVertexType();
				u32 vertexCount = buffer->getVertexCount();

				if (hasSecondTextureCoordinates(vtxType))
				{
					globalIndices[i].TCoord1StartIndex = 0;

					if (i!=0 && globalIndices[i-1].TCoord1LastIndex != -1)
						globalIndices[i].TCoord1StartIndex = globalIndices[i-1].TCoord1LastIndex + 1;

					globalIndices[i].TCoord1LastIndex = globalIndices[i].TCoord1StartIndex + vertexCount - 1;

					switch(vtxType)
					{
					case video::EVT_2TCOORDS:
						{
							video::S3DVertex2TCoords* vtx = (video::S3DVertex2TCoords*)buffer->getVertices();
							for (u32 j=0; j<vertexCount; ++j)
							{
								writeVector(vtx[j].TCoords2);
								Writer->writeLineBreak();
							}
						}
						break;
					default:
						break;
					}
				} // end this buffer has 2 texture coordinates
			}

			Writer->writeClosingTag(L"float_array");
			Writer->writeLineBreak();

			Writer->writeElement(L"technique_common", false);
			Writer->writeLineBreak();

			vertexCountStr = core::stringw(totalTCoords2Count);

				Writer->writeElement(L"accessor", false, L"source", toRef(meshTexCoord1ArrayId).c_str(),
										L"count", vertexCountStr.c_str(), L"stride", L"2");
				Writer->writeLineBreak();

					Writer->writeElement(L"param", true, L"name", L"U", L"type", L"float");
					Writer->writeLineBreak();
					Writer->writeElement(L"param", true, L"name", L"V", L"type", L"float");
					Writer->writeLineBreak();

				Writer->writeClosingTag(L"accessor");
				Writer->writeLineBreak();

			Writer->writeClosingTag(L"technique_common");
			Writer->writeLineBreak();

		Writer->writeClosingTag(L"source");
		Writer->writeLineBreak();
	}

	// write tangents

	// TODO

	// write vertices
	core::stringw meshVtxId(meshId);
	meshVtxId += L"-Vtx";
	Writer->writeElement(L"vertices", false, L"id", meshVtxId.c_str());
	Writer->writeLineBreak();

		Writer->writeElement(L"input", true, L"semantic", L"POSITION", L"source", toRef(meshPosId).c_str());
		Writer->writeLineBreak();

	Writer->writeClosingTag(L"vertices");
	Writer->writeLineBreak();

	// write polygons

	for (i=0; i<mesh->getMeshBufferCount(); ++i)
	{
		scene::IMeshBuffer* buffer = mesh->getMeshBuffer(i);

		const u32 polyCount = buffer->getIndexCount() / 3;
		core::stringw strPolyCount(polyCount);
		irr::core::stringw strMat(nameForMaterialSymbol(mesh, i));

		Writer->writeElement(L"triangles", false, L"count", strPolyCount.c_str(),
								L"material", strMat.c_str());
		Writer->writeLineBreak();

		Writer->writeElement(L"input", true, L"semantic", L"VERTEX", L"source", toRef(meshVtxId).c_str(), L"offset", L"0");
		Writer->writeLineBreak();
		Writer->writeElement(L"input", true, L"semantic", L"TEXCOORD", L"source", toRef(meshTexCoord0Id).c_str(), L"offset", L"1", L"set", L"0");
		Writer->writeLineBreak();
		Writer->writeElement(L"input", true, L"semantic", L"NORMAL", L"source", toRef(meshNormalId).c_str(), L"offset", L"2");
		Writer->writeLineBreak();

		bool has2ndTexCoords = hasSecondTextureCoordinates(buffer->getVertexType());
		if (has2ndTexCoords)
		{
			// TODO: when working on second uv-set - my suspicion is that this one should be called "TEXCOORD2"
			// to allow bind_vertex_input to differentiate the uv-sets.
			Writer->writeElement(L"input", true, L"semantic", L"TEXCOORD", L"source", toRef(meshTexCoord1Id).c_str(), L"idx", L"3");
			Writer->writeLineBreak();
		}

		// write indices now

		s32 posIdx = globalIndices[i].PosStartIndex;
		s32 tCoordIdx = globalIndices[i].TCoord0StartIndex;
		s32 normalIdx = globalIndices[i].NormalStartIndex;
		s32 tCoord2Idx = globalIndices[i].TCoord1StartIndex;

		Writer->writeElement(L"p", false);

		core::stringw strP;
		strP.reserve(100);
		for (u32 p=0; p<polyCount; ++p)
		{
			strP = "";
			strP += buffer->getIndices()[(p*3) + 0] + posIdx;
			strP += " ";
			strP += buffer->getIndices()[(p*3) + 0] + tCoordIdx;
			strP += " ";
			strP += buffer->getIndices()[(p*3) + 0] + normalIdx;
			strP += " ";
			if (has2ndTexCoords)
			{
				strP += buffer->getIndices()[(p*3) + 0] + tCoord2Idx;
				strP += " ";
			}

			strP += buffer->getIndices()[(p*3) + 1] + posIdx;
			strP += " ";
			strP += buffer->getIndices()[(p*3) + 1] + tCoordIdx;
			strP += " ";
			strP += buffer->getIndices()[(p*3) + 1] + normalIdx;
			strP += " ";
			if (has2ndTexCoords)
			{
				strP += buffer->getIndices()[(p*3) + 1] + tCoord2Idx;
				strP += " ";
			}

			strP += buffer->getIndices()[(p*3) + 2] + posIdx;
			strP += " ";
			strP += buffer->getIndices()[(p*3) + 2] + tCoordIdx;
			strP += " ";
			strP += buffer->getIndices()[(p*3) + 2] + normalIdx;
			if (has2ndTexCoords)
			{
				strP += " ";
				strP += buffer->getIndices()[(p*3) + 2] + tCoord2Idx;
			}
			strP += " ";

			Writer->writeText(strP.c_str());
		}

		Writer->writeClosingTag(L"p");
		Writer->writeLineBreak();

		// close index buffer section

		Writer->writeClosingTag(L"triangles");
		Writer->writeLineBreak();
	}

	// close mesh and geometry
	delete [] globalIndices;
	Writer->writeClosingTag(L"mesh");
	Writer->writeLineBreak();
	Writer->writeClosingTag(L"geometry");
	Writer->writeLineBreak();
}

void CColladaMeshWriter::writeLibraryImages()
{
	if ( getWriteTextures() && !LibraryImages.empty() )
	{
		Writer->writeElement(L"library_images", false);
		Writer->writeLineBreak();

		for ( irr::u32 i=0; i<LibraryImages.size(); ++i )
		{
			irr::io::path p(FileSystem->getRelativeFilename(LibraryImages[i]->getName().getPath(), Directory));
			//<image name="rose01">
			irr::core::stringw ncname( toNCName(irr::core::stringw(p)) );
			Writer->writeElement(L"image", false, L"id", ncname.c_str(), L"name", ncname.c_str());
			Writer->writeLineBreak();
			//  <init_from>../flowers/rose01.jpg</init_from>
				Writer->writeElement(L"init_from", false);
				Writer->writeText(pathToURI(p).c_str());
				Writer->writeClosingTag(L"init_from");
				Writer->writeLineBreak();
	 		//  </image>
			Writer->writeClosingTag(L"image");
			Writer->writeLineBreak();
		}

		Writer->writeClosingTag(L"library_images");
		Writer->writeLineBreak();
	}
}

void CColladaMeshWriter::writeColorElement(const video::SColorf & col, bool writeAlpha)
{
	Writer->writeElement(L"color", false);

	writeColor(col, writeAlpha);

	Writer->writeClosingTag(L"color");
	Writer->writeLineBreak();
}

void CColladaMeshWriter::writeColorElement(const video::SColor & col, bool writeAlpha)
{
	writeColorElement( video::SColorf(col), writeAlpha );
}

void CColladaMeshWriter::writeAmbientLightElement(const video::SColorf & col)
{
	Writer->writeElement(L"light", false, L"id", L"ambientlight");
	Writer->writeLineBreak();

		Writer->writeElement(L"technique_common", false);
		Writer->writeLineBreak();

			Writer->writeElement(L"ambient", false);
			Writer->writeLineBreak();

				writeColorElement(col, false);

			Writer->writeClosingTag(L"ambient");
			Writer->writeLineBreak();

		Writer->writeClosingTag(L"technique_common");
		Writer->writeLineBreak();

	Writer->writeClosingTag(L"light");
	Writer->writeLineBreak();
}

s32 CColladaMeshWriter::getCheckedTextureIdx(const video::SMaterial & material, E_COLLADA_COLOR_SAMPLER cs)
{
	if (	!getWriteTextures()
		||	!getProperties() )
		return -1;

	s32 idx = getProperties()->getTextureIdx(material, cs);
	if ( idx >= 0 && !material.TextureLayer[idx].Texture )
		return -1;

	return idx;
}

video::SColor CColladaMeshWriter::getColorMapping(const video::SMaterial & material, E_COLLADA_COLOR_SAMPLER cs, E_COLLADA_IRR_COLOR colType)
{
	switch ( colType )
	{
		case ECIC_NONE:
			return video::SColor(255, 0, 0, 0);

		case ECIC_CUSTOM:
			return getProperties()->getCustomColor(material, cs);

		case ECIC_DIFFUSE:
			return material.DiffuseColor;

		case ECIC_AMBIENT:
			return material.AmbientColor;

		case ECIC_EMISSIVE:
			return material.EmissiveColor;

		case ECIC_SPECULAR:
			return material.SpecularColor;
	}
	return video::SColor(255, 0, 0, 0);
}

void CColladaMeshWriter::writeTextureSampler(s32 textureIdx)
{
	irr::core::stringw sampler(L"tex");
	sampler += irr::core::stringw(textureIdx);
	sampler += L"-sampler";

	// <texture texture="sampler" texcoord="texCoordUv"/>
	Writer->writeElement(L"texture", true, L"texture", sampler.c_str(), L"texcoord", L"uv" );
	Writer->writeLineBreak();
}

void CColladaMeshWriter::writeFxElement(const video::SMaterial & material, E_COLLADA_TECHNIQUE_FX techFx)
{
	core::stringw fxLabel;
	bool writeEmission = true;
	bool writeAmbient = true;
	bool writeDiffuse = true;
	bool writeSpecular = true;
	bool writeShininess = true;
	bool writeReflective = true;
	bool writeReflectivity = true;
	bool writeTransparent = true;
	bool writeTransparency = true;
	bool writeIndexOfRefraction = true;
	switch ( techFx )
	{
		case ECTF_BLINN:
			fxLabel = L"blinn";
			break;
		case ECTF_PHONG:
			fxLabel = L"phong";
			break;
		case ECTF_LAMBERT:
			fxLabel = L"lambert";
			writeSpecular = false;
			writeShininess = false;
			break;
		case ECTF_CONSTANT:
			fxLabel = L"constant";
			writeAmbient = false;
			writeDiffuse = false;
			writeSpecular = false;
			writeShininess = false;
			break;
	}

	Writer->writeElement(fxLabel.c_str(), false);
	Writer->writeLineBreak();

	// write all interesting material parameters
	// attributes must be written in fixed order
	if ( getProperties() )
	{
		if ( writeEmission )
		{
			writeColorFx(material, L"emission", ECCS_EMISSIVE);
		}

		if ( writeAmbient )
		{
			writeColorFx(material, L"ambient", ECCS_AMBIENT);
		}

		if ( writeDiffuse )
		{
			writeColorFx(material, L"diffuse", ECCS_DIFFUSE);
		}

		if ( writeSpecular )
		{
			writeColorFx(material, L"specular", ECCS_SPECULAR);
		}

		if ( writeShininess )
		{
			Writer->writeElement(L"shininess", false);
			Writer->writeLineBreak();
			writeFloatElement(material.Shininess);
			Writer->writeClosingTag(L"shininess");
			Writer->writeLineBreak();
		}

		if ( writeReflective )
		{
			writeColorFx(material, L"reflective", ECCS_REFLECTIVE);
		}

		if ( writeReflectivity )
		{
			f32 t = getProperties()->getReflectivity(material);
			if ( t >= 0.f )
			{
				// <transparency>  <float>1.000000</float> </transparency>
				Writer->writeElement(L"reflectivity", false);
				Writer->writeLineBreak();
				writeFloatElement(t);
				Writer->writeClosingTag(L"reflectivity");
				Writer->writeLineBreak();
			}
		}

		if ( writeTransparent )
		{
			E_COLLADA_TRANSPARENT_FX transparentFx = getProperties()->getTransparentFx(material);
			writeColorFx(material, L"transparent", ECCS_TRANSPARENT, L"opaque", toString(transparentFx).c_str());
		}

		if ( writeTransparency  )
		{
			f32 t = getProperties()->getTransparency(material);
			if ( t >= 0.f )
			{
				// <transparency>  <float>1.000000</float> </transparency>
				Writer->writeElement(L"transparency", false);
				Writer->writeLineBreak();
				writeFloatElement(t);
				Writer->writeClosingTag(L"transparency");
				Writer->writeLineBreak();
			}
		}

		if ( writeIndexOfRefraction )
		{
			f32 t = getProperties()->getIndexOfRefraction(material);
			if ( t >= 0.f )
			{
				Writer->writeElement(L"index_of_refraction", false);
				Writer->writeLineBreak();
				writeFloatElement(t);
				Writer->writeClosingTag(L"index_of_refraction");
				Writer->writeLineBreak();
			}
		}
	}


	Writer->writeClosingTag(fxLabel.c_str());
	Writer->writeLineBreak();
}

void CColladaMeshWriter::writeColorFx(const video::SMaterial & material, const wchar_t * colorname, E_COLLADA_COLOR_SAMPLER cs, const wchar_t* attr1Name, const wchar_t* attr1Value)
{
	irr::s32 idx = getCheckedTextureIdx(material, cs);
	E_COLLADA_IRR_COLOR colType = idx < 0 ? getProperties()->getColorMapping(material, cs) : ECIC_NONE;
	if ( idx >= 0 || colType != ECIC_NONE )
	{
		Writer->writeElement(colorname, false, attr1Name, attr1Value);
		Writer->writeLineBreak();
		if ( idx >= 0 )
			writeTextureSampler(idx);
		else
			writeColorElement(getColorMapping(material, cs, colType));
		Writer->writeClosingTag(colorname);
		Writer->writeLineBreak();
	}
}

void CColladaMeshWriter::writeNode(const wchar_t * nodeName, const wchar_t * content)
{
	Writer->writeElement(nodeName, false);
	Writer->writeText(content);
	Writer->writeClosingTag(nodeName);
	Writer->writeLineBreak();
}

void CColladaMeshWriter::writeFloatElement(irr::f32 value)
{
	Writer->writeElement(L"float", false);
	Writer->writeText(core::stringw((double)value).c_str());
	Writer->writeClosingTag(L"float");
	Writer->writeLineBreak();
}

void CColladaMeshWriter::writeRotateElement(const irr::core::vector3df& axis, irr::f32 angle)
{
	Writer->writeElement(L"rotate", false);
	irr::core::stringw txt(axis.X);
	txt += L" ";
	txt += irr::core::stringw(axis.Y);
	txt += L" ";
	txt += irr::core::stringw(axis.Z);
	txt += L" ";
	txt += irr::core::stringw((double)angle);
	Writer->writeText(txt.c_str());
	Writer->writeClosingTag(L"rotate");
	Writer->writeLineBreak();
}

void CColladaMeshWriter::writeScaleElement(const irr::core::vector3df& scale)
{
	Writer->writeElement(L"scale", false);
	irr::core::stringw txt(scale.X);
	txt += L" ";
	txt += irr::core::stringw(scale.Y);
	txt += L" ";
	txt += irr::core::stringw(scale.Z);
	Writer->writeText(txt.c_str());
	Writer->writeClosingTag(L"scale");
	Writer->writeLineBreak();
}

void CColladaMeshWriter::writeTranslateElement(const irr::core::vector3df& translate)
{
	Writer->writeElement(L"translate", false);
	irr::core::stringw txt(translate.X);
	txt += L" ";
	txt += irr::core::stringw(translate.Y);
	txt += L" ";
	txt += irr::core::stringw(translate.Z);
	Writer->writeText(txt.c_str());
	Writer->writeClosingTag(L"translate");
	Writer->writeLineBreak();
}

void CColladaMeshWriter::writeMatrixElement(const irr::core::matrix4& matrix)
{
	Writer->writeElement(L"matrix", false);
	Writer->writeLineBreak();

	for ( int a=0; a<4; ++a )
	{
		irr::core::stringw txt;
		for ( int b=0; b<4; ++b )
		{
			if ( b > 0 )
				txt += " ";
			// row-column switched compared to Irrlicht
			txt += irr::core::stringw(matrix[b*4+a]);
		}
		Writer->writeText(txt.c_str());
		Writer->writeLineBreak();
	}

	Writer->writeClosingTag(L"matrix");
	Writer->writeLineBreak();
}

} // end namespace
} // end namespace

#endif

