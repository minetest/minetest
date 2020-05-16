// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "IrrCompileConfig.h"
#include "CSceneManager.h"
#include "IVideoDriver.h"
#include "IFileSystem.h"
#include "SAnimatedMesh.h"
#include "CMeshCache.h"
#include "IXMLWriter.h"
#include "ISceneUserDataSerializer.h"
#include "IGUIEnvironment.h"
#include "IMaterialRenderer.h"
#include "IReadFile.h"
#include "IWriteFile.h"
#include "ISceneLoader.h"

#include "os.h"

// We need this include for the case of skinned mesh support without
// any such loader
#ifdef _IRR_COMPILE_WITH_SKINNED_MESH_SUPPORT_
#include "CSkinnedMesh.h"
#endif

#ifdef _IRR_COMPILE_WITH_IRR_MESH_LOADER_
#include "CIrrMeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_BSP_LOADER_
#include "CBSPMeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_MD2_LOADER_
#include "CMD2MeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_HALFLIFE_LOADER_
#include "CAnimatedMeshHalfLife.h"
#endif

#ifdef _IRR_COMPILE_WITH_MS3D_LOADER_
#include "CMS3DMeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_3DS_LOADER_
#include "C3DSMeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_X_LOADER_
#include "CXMeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_OCT_LOADER_
#include "COCTLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_CSM_LOADER_
#include "CCSMLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_LMTS_LOADER_
#include "CLMTSMeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_MY3D_LOADER_
#include "CMY3DMeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_COLLADA_LOADER_
#include "CColladaFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_DMF_LOADER_
#include "CDMFLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_OGRE_LOADER_
#include "COgreMeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_OBJ_LOADER_
#include "COBJMeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_MD3_LOADER_
#include "CMD3MeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_B3D_LOADER_
#include "CB3DMeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_LWO_LOADER_
#include "CLWOMeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_STL_LOADER_
#include "CSTLMeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_PLY_LOADER_
#include "CPLYMeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_SMF_LOADER_
#include "CSMFMeshFileLoader.h"
#endif

#ifdef _IRR_COMPILE_WITH_IRR_SCENE_LOADER_
#include "CSceneLoaderIrr.h"
#endif

#ifdef _IRR_COMPILE_WITH_COLLADA_WRITER_
#include "CColladaMeshWriter.h"
#endif

#ifdef _IRR_COMPILE_WITH_IRR_WRITER_
#include "CIrrMeshWriter.h"
#endif

#ifdef _IRR_COMPILE_WITH_STL_WRITER_
#include "CSTLMeshWriter.h"
#endif

#ifdef _IRR_COMPILE_WITH_OBJ_WRITER_
#include "COBJMeshWriter.h"
#endif

#ifdef _IRR_COMPILE_WITH_PLY_WRITER_
#include "CPLYMeshWriter.h"
#endif

#include "CCubeSceneNode.h"
#include "CSphereSceneNode.h"
#include "CAnimatedMeshSceneNode.h"
#include "COctreeSceneNode.h"
#include "CCameraSceneNode.h"
#include "CLightSceneNode.h"
#include "CBillboardSceneNode.h"
#include "CMeshSceneNode.h"
#include "CSkyBoxSceneNode.h"
#include "CSkyDomeSceneNode.h"
#include "CParticleSystemSceneNode.h"
#include "CDummyTransformationSceneNode.h"
#include "CWaterSurfaceSceneNode.h"
#include "CTerrainSceneNode.h"
#include "CEmptySceneNode.h"
#include "CTextSceneNode.h"
#include "CQuake3ShaderSceneNode.h"
#include "CVolumeLightSceneNode.h"

#include "CDefaultSceneNodeFactory.h"

#include "CSceneCollisionManager.h"
#include "CTriangleSelector.h"
#include "COctreeTriangleSelector.h"
#include "CTriangleBBSelector.h"
#include "CMetaTriangleSelector.h"
#include "CTerrainTriangleSelector.h"

#include "CSceneNodeAnimatorRotation.h"
#include "CSceneNodeAnimatorFlyCircle.h"
#include "CSceneNodeAnimatorFlyStraight.h"
#include "CSceneNodeAnimatorTexture.h"
#include "CSceneNodeAnimatorCollisionResponse.h"
#include "CSceneNodeAnimatorDelete.h"
#include "CSceneNodeAnimatorFollowSpline.h"
#include "CSceneNodeAnimatorCameraFPS.h"
#include "CSceneNodeAnimatorCameraMaya.h"
#include "CDefaultSceneNodeAnimatorFactory.h"

#include "CGeometryCreator.h"

namespace irr
{
namespace scene
{

//! constructor
CSceneManager::CSceneManager(video::IVideoDriver* driver, io::IFileSystem* fs,
		gui::ICursorControl* cursorControl, IMeshCache* cache,
		gui::IGUIEnvironment* gui)
: ISceneNode(0, 0), Driver(driver), FileSystem(fs), GUIEnvironment(gui),
	CursorControl(cursorControl), CollisionManager(0),
	ActiveCamera(0), ShadowColor(150,0,0,0), AmbientLight(0,0,0,0),
	MeshCache(cache), CurrentRendertime(ESNRP_NONE), LightManager(0),
	IRR_XML_FORMAT_SCENE(L"irr_scene"), IRR_XML_FORMAT_NODE(L"node"), IRR_XML_FORMAT_NODE_ATTR_TYPE(L"type")
{
	#ifdef _DEBUG
	ISceneManager::setDebugName("CSceneManager ISceneManager");
	ISceneNode::setDebugName("CSceneManager ISceneNode");
	#endif

	// root node's scene manager
	SceneManager = this;

	// set scene parameters
	Parameters.setAttribute( DEBUG_NORMAL_LENGTH, 1.f );
	Parameters.setAttribute( DEBUG_NORMAL_COLOR, video::SColor(255, 34, 221, 221));

	if (Driver)
		Driver->grab();

	if (FileSystem)
		FileSystem->grab();

	if (CursorControl)
		CursorControl->grab();

	if (GUIEnvironment)
		GUIEnvironment->grab();

	// create mesh cache if not there already
	if (!MeshCache)
		MeshCache = new CMeshCache();
	else
		MeshCache->grab();

	// create collision manager
	CollisionManager = new CSceneCollisionManager(this, Driver);

	// create geometry creator
	GeometryCreator = new CGeometryCreator();

	// add file format loaders. add the least commonly used ones first,
	// as these are checked last

	// TODO: now that we have multiple scene managers, these should be
	// shallow copies from the previous manager if there is one.

	#ifdef _IRR_COMPILE_WITH_STL_LOADER_
	MeshLoaderList.push_back(new CSTLMeshFileLoader());
	#endif
	#ifdef _IRR_COMPILE_WITH_PLY_LOADER_
	MeshLoaderList.push_back(new CPLYMeshFileLoader(this));
	#endif
	#ifdef _IRR_COMPILE_WITH_SMF_LOADER_
	MeshLoaderList.push_back(new CSMFMeshFileLoader(Driver));
	#endif
	#ifdef _IRR_COMPILE_WITH_OCT_LOADER_
	MeshLoaderList.push_back(new COCTLoader(this, FileSystem));
	#endif
	#ifdef _IRR_COMPILE_WITH_CSM_LOADER_
	MeshLoaderList.push_back(new CCSMLoader(this, FileSystem));
	#endif
	#ifdef _IRR_COMPILE_WITH_LMTS_LOADER_
	MeshLoaderList.push_back(new CLMTSMeshFileLoader(FileSystem, Driver, &Parameters));
	#endif
	#ifdef _IRR_COMPILE_WITH_MY3D_LOADER_
	MeshLoaderList.push_back(new CMY3DMeshFileLoader(this, FileSystem));
	#endif
	#ifdef _IRR_COMPILE_WITH_DMF_LOADER_
	MeshLoaderList.push_back(new CDMFLoader(this, FileSystem));
	#endif
	#ifdef _IRR_COMPILE_WITH_OGRE_LOADER_
	MeshLoaderList.push_back(new COgreMeshFileLoader(FileSystem, Driver));
	#endif
	#ifdef _IRR_COMPILE_WITH_HALFLIFE_LOADER_
	MeshLoaderList.push_back(new CHalflifeMDLMeshFileLoader( this ));
	#endif
	#ifdef _IRR_COMPILE_WITH_MD3_LOADER_
	MeshLoaderList.push_back(new CMD3MeshFileLoader( this));
	#endif
	#ifdef _IRR_COMPILE_WITH_LWO_LOADER_
	MeshLoaderList.push_back(new CLWOMeshFileLoader(this, FileSystem));
	#endif
	#ifdef _IRR_COMPILE_WITH_MD2_LOADER_
	MeshLoaderList.push_back(new CMD2MeshFileLoader());
	#endif
	#ifdef _IRR_COMPILE_WITH_IRR_MESH_LOADER_
	MeshLoaderList.push_back(new CIrrMeshFileLoader(this, FileSystem));
	#endif
	#ifdef _IRR_COMPILE_WITH_BSP_LOADER_
	MeshLoaderList.push_back(new CBSPMeshFileLoader(this, FileSystem));
	#endif
	#ifdef _IRR_COMPILE_WITH_COLLADA_LOADER_
	MeshLoaderList.push_back(new CColladaFileLoader(this, FileSystem));
	#endif
	#ifdef _IRR_COMPILE_WITH_3DS_LOADER_
	MeshLoaderList.push_back(new C3DSMeshFileLoader(this, FileSystem));
	#endif
	#ifdef _IRR_COMPILE_WITH_X_LOADER_
	MeshLoaderList.push_back(new CXMeshFileLoader(this, FileSystem));
	#endif
	#ifdef _IRR_COMPILE_WITH_MS3D_LOADER_
	MeshLoaderList.push_back(new CMS3DMeshFileLoader(Driver));
	#endif
	#ifdef _IRR_COMPILE_WITH_OBJ_LOADER_
	MeshLoaderList.push_back(new COBJMeshFileLoader(this, FileSystem));
	#endif
	#ifdef _IRR_COMPILE_WITH_B3D_LOADER_
	MeshLoaderList.push_back(new CB3DMeshFileLoader(this));
	#endif

	// scene loaders
	#ifdef _IRR_COMPILE_WITH_IRR_SCENE_LOADER_
	SceneLoaderList.push_back(new CSceneLoaderIrr(this, FileSystem));
	#endif


	// factories
	ISceneNodeFactory* factory = new CDefaultSceneNodeFactory(this);
	registerSceneNodeFactory(factory);
	factory->drop();

	ISceneNodeAnimatorFactory* animatorFactory = new CDefaultSceneNodeAnimatorFactory(this, CursorControl);
	registerSceneNodeAnimatorFactory(animatorFactory);
	animatorFactory->drop();
}


//! destructor
CSceneManager::~CSceneManager()
{
	clearDeletionList();

	//! force to remove hardwareTextures from the driver
	//! because Scenes may hold internally data bounded to sceneNodes
	//! which may be destroyed twice
	if (Driver)
		Driver->removeAllHardwareBuffers();

	if (FileSystem)
		FileSystem->drop();

	if (CursorControl)
		CursorControl->drop();

	if (CollisionManager)
		CollisionManager->drop();

	if (GeometryCreator)
		GeometryCreator->drop();

	if (GUIEnvironment)
		GUIEnvironment->drop();

	u32 i;
	for (i=0; i<MeshLoaderList.size(); ++i)
		MeshLoaderList[i]->drop();

	for (i=0; i<SceneLoaderList.size(); ++i)
		SceneLoaderList[i]->drop();

	if (ActiveCamera)
		ActiveCamera->drop();
	ActiveCamera = 0;

	if (MeshCache)
		MeshCache->drop();

	for (i=0; i<SceneNodeFactoryList.size(); ++i)
		SceneNodeFactoryList[i]->drop();

	for (i=0; i<SceneNodeAnimatorFactoryList.size(); ++i)
		SceneNodeAnimatorFactoryList[i]->drop();

	if (LightManager)
		LightManager->drop();

	// remove all nodes and animators before dropping the driver
	// as render targets may be destroyed twice

	removeAll();
	removeAnimators();

	if (Driver)
		Driver->drop();
}


//! gets an animateable mesh. loads it if needed. returned pointer must not be dropped.
IAnimatedMesh* CSceneManager::getMesh(const io::path& filename)
{
	IAnimatedMesh* msh = MeshCache->getMeshByName(filename);
	if (msh)
		return msh;

	io::IReadFile* file = FileSystem->createAndOpenFile(filename);
	if (!file)
	{
		os::Printer::log("Could not load mesh, because file could not be opened: ", filename, ELL_ERROR);
		return 0;
	}

	// iterate the list in reverse order so user-added loaders can override the built-in ones
	s32 count = MeshLoaderList.size();
	for (s32 i=count-1; i>=0; --i)
	{
		if (MeshLoaderList[i]->isALoadableFileExtension(filename))
		{
			// reset file to avoid side effects of previous calls to createMesh
			file->seek(0);
			msh = MeshLoaderList[i]->createMesh(file);
			if (msh)
			{
				MeshCache->addMesh(filename, msh);
				msh->drop();
				break;
			}
		}
	}

	file->drop();

	if (!msh)
		os::Printer::log("Could not load mesh, file format seems to be unsupported", filename, ELL_ERROR);
	else
		os::Printer::log("Loaded mesh", filename, ELL_INFORMATION);

	return msh;
}


//! gets an animateable mesh. loads it if needed. returned pointer must not be dropped.
IAnimatedMesh* CSceneManager::getMesh(io::IReadFile* file)
{
	if (!file)
		return 0;

	io::path name = file->getFileName();
	IAnimatedMesh* msh = MeshCache->getMeshByName(file->getFileName());
	if (msh)
		return msh;

	// iterate the list in reverse order so user-added loaders can override the built-in ones
	s32 count = MeshLoaderList.size();
	for (s32 i=count-1; i>=0; --i)
	{
		if (MeshLoaderList[i]->isALoadableFileExtension(name))
		{
			// reset file to avoid side effects of previous calls to createMesh
			file->seek(0);
			msh = MeshLoaderList[i]->createMesh(file);
			if (msh)
			{
				MeshCache->addMesh(file->getFileName(), msh);
				msh->drop();
				break;
			}
		}
	}

	if (!msh)
		os::Printer::log("Could not load mesh, file format seems to be unsupported", file->getFileName(), ELL_ERROR);
	else
		os::Printer::log("Loaded mesh", file->getFileName(), ELL_INFORMATION);

	return msh;
}


//! returns the video driver
video::IVideoDriver* CSceneManager::getVideoDriver()
{
	return Driver;
}


//! returns the GUI Environment
gui::IGUIEnvironment* CSceneManager::getGUIEnvironment()
{
	return GUIEnvironment;
}

//! Get the active FileSystem
/** \return Pointer to the FileSystem
This pointer should not be dropped. See IReferenceCounted::drop() for more information. */
io::IFileSystem* CSceneManager::getFileSystem()
{
	return FileSystem;
}

//! Adds a text scene node, which is able to display
//! 2d text at a position in three dimensional space
ITextSceneNode* CSceneManager::addTextSceneNode(gui::IGUIFont* font,
		const wchar_t* text, video::SColor color, ISceneNode* parent,
		const core::vector3df& position, s32 id)
{
	if (!font)
		return 0;

	if (!parent)
		parent = this;

	ITextSceneNode* t = new CTextSceneNode(parent, this, id, font,
		getSceneCollisionManager(), position, text, color);
	t->drop();

	return t;
}


//! Adds a text scene node, which uses billboards
IBillboardTextSceneNode* CSceneManager::addBillboardTextSceneNode(gui::IGUIFont* font,
		const wchar_t* text, ISceneNode* parent,
		const core::dimension2d<f32>& size,
		const core::vector3df& position, s32 id,
		video::SColor colorTop, video::SColor colorBottom)
{
	if (!font && GUIEnvironment)
		font = GUIEnvironment->getBuiltInFont();

	if (!font)
		return 0;

	if (!parent)
		parent = this;

	IBillboardTextSceneNode* node = new CBillboardTextSceneNode(parent, this, id, font, text, position, size,
		colorTop, colorBottom);
	node->drop();

	return node;

}


//! Adds a scene node, which can render a quake3 shader
IMeshSceneNode* CSceneManager::addQuake3SceneNode(const IMeshBuffer* meshBuffer,
					const quake3::IShader * shader,
					ISceneNode* parent, s32 id )
{
#ifdef _IRR_COMPILE_WITH_BSP_LOADER_
	if (!shader)
		return 0;

	if (!parent)
		parent = this;

	CQuake3ShaderSceneNode* node = new CQuake3ShaderSceneNode( parent,
		this, id, FileSystem,
		meshBuffer, shader );
	node->drop();

	return node;
#else
	return 0;
#endif
}


//! adds Volume Lighting Scene Node.
//! the returned pointer must not be dropped.
IVolumeLightSceneNode* CSceneManager::addVolumeLightSceneNode(
		ISceneNode* parent, s32 id,
		const u32 subdivU, const u32 subdivV,
		const video::SColor foot, const video::SColor tail,
		const core::vector3df& position, const core::vector3df& rotation, const core::vector3df& scale)
{
	if (!parent)
		parent = this;

	IVolumeLightSceneNode* node = new CVolumeLightSceneNode(parent, this, id, subdivU, subdivV, foot, tail, position, rotation, scale);
	node->drop();

	return node;
}


//! adds a test scene node for test purposes to the scene. It is a simple cube of (1,1,1) size.
//! the returned pointer must not be dropped.
IMeshSceneNode* CSceneManager::addCubeSceneNode(f32 size, ISceneNode* parent,
		s32 id, const core::vector3df& position,
		const core::vector3df& rotation, const core::vector3df& scale)
{
	if (!parent)
		parent = this;

	IMeshSceneNode* node = new CCubeSceneNode(size, parent, this, id, position, rotation, scale);
	node->drop();

	return node;
}


//! Adds a sphere scene node for test purposes to the scene.
IMeshSceneNode* CSceneManager::addSphereSceneNode(f32 radius, s32 polyCount,
		ISceneNode* parent, s32 id, const core::vector3df& position,
		const core::vector3df& rotation, const core::vector3df& scale)
{
	if (!parent)
		parent = this;

	IMeshSceneNode* node = new CSphereSceneNode(radius, polyCount, polyCount, parent, this, id, position, rotation, scale);
	node->drop();

	return node;
}


//! adds a scene node for rendering a static mesh
//! the returned pointer must not be dropped.
IMeshSceneNode* CSceneManager::addMeshSceneNode(IMesh* mesh, ISceneNode* parent, s32 id,
	const core::vector3df& position, const core::vector3df& rotation,
	const core::vector3df& scale, bool alsoAddIfMeshPointerZero)
{
	if (!alsoAddIfMeshPointerZero && !mesh)
		return 0;

	if (!parent)
		parent = this;

	IMeshSceneNode* node = new CMeshSceneNode(mesh, parent, this, id, position, rotation, scale);
	node->drop();

	return node;
}


//! Adds a scene node for rendering a animated water surface mesh.
ISceneNode* CSceneManager::addWaterSurfaceSceneNode(IMesh* mesh, f32 waveHeight, f32 waveSpeed, f32 waveLength,
	ISceneNode* parent, s32 id, const core::vector3df& position,
	const core::vector3df& rotation, const core::vector3df& scale)
{
	if (!parent)
		parent = this;

	ISceneNode* node = new CWaterSurfaceSceneNode(waveHeight, waveSpeed, waveLength,
		mesh, parent, this, id, position, rotation, scale);

	node->drop();

	return node;
}


//! adds a scene node for rendering an animated mesh model
IAnimatedMeshSceneNode* CSceneManager::addAnimatedMeshSceneNode(IAnimatedMesh* mesh, ISceneNode* parent, s32 id,
	const core::vector3df& position, const core::vector3df& rotation,
	const core::vector3df& scale, bool alsoAddIfMeshPointerZero)
{
	if (!alsoAddIfMeshPointerZero && !mesh)
		return 0;

	if (!parent)
		parent = this;

	IAnimatedMeshSceneNode* node =
		new CAnimatedMeshSceneNode(mesh, parent, this, id, position, rotation, scale);
	node->drop();

	return node;
}


//! Adds a scene node for rendering using a octree to the scene graph. This a good method for rendering
//! scenes with lots of geometry. The Octree is built on the fly from the mesh, much
//! faster then a bsp tree.
IMeshSceneNode* CSceneManager::addOctreeSceneNode(IAnimatedMesh* mesh, ISceneNode* parent,
			s32 id, s32 minimalPolysPerNode, bool alsoAddIfMeshPointerZero)
{
	if (!alsoAddIfMeshPointerZero && (!mesh || !mesh->getFrameCount()))
		return 0;

	return addOctreeSceneNode(mesh ? mesh->getMesh(0) : 0,
				parent, id, minimalPolysPerNode,
				alsoAddIfMeshPointerZero);
}


//! Adds a scene node for rendering using a octree. This a good method for rendering
//! scenes with lots of geometry. The Octree is built on the fly from the mesh, much
//! faster then a bsp tree.
IMeshSceneNode* CSceneManager::addOctreeSceneNode(IMesh* mesh, ISceneNode* parent,
		s32 id, s32 minimalPolysPerNode, bool alsoAddIfMeshPointerZero)
{
	if (!alsoAddIfMeshPointerZero && !mesh)
		return 0;

	if (!parent)
		parent = this;

	COctreeSceneNode* node = new COctreeSceneNode(parent, this, id, minimalPolysPerNode);

	if (node)
	{
		node->setMesh(mesh);
		node->drop();
	}

	return node;
}


//! Adds a camera scene node to the tree and sets it as active camera.
//! \param position: Position of the space relative to its parent where the camera will be placed.
//! \param lookat: Position where the camera will look at. Also known as target.
//! \param parent: Parent scene node of the camera. Can be null. If the parent moves,
//! the camera will move too.
//! \return Returns pointer to interface to camera
ICameraSceneNode* CSceneManager::addCameraSceneNode(ISceneNode* parent,
	const core::vector3df& position, const core::vector3df& lookat, s32 id,
	bool makeActive)
{
	if (!parent)
		parent = this;

	ICameraSceneNode* node = new CCameraSceneNode(parent, this, id, position, lookat);

	if (makeActive)
		setActiveCamera(node);
	node->drop();

	return node;
}


//! Adds a camera scene node which is able to be controlled with the mouse similar
//! to in the 3D Software Maya by Alias Wavefront.
//! The returned pointer must not be dropped.
ICameraSceneNode* CSceneManager::addCameraSceneNodeMaya(ISceneNode* parent,
	f32 rotateSpeed, f32 zoomSpeed, f32 translationSpeed, s32 id, f32 distance,
	bool makeActive)
{
	ICameraSceneNode* node = addCameraSceneNode(parent, core::vector3df(),
			core::vector3df(0,0,100), id, makeActive);
	if (node)
	{
		ISceneNodeAnimator* anm = new CSceneNodeAnimatorCameraMaya(CursorControl,
			rotateSpeed, zoomSpeed, translationSpeed, distance);

		node->addAnimator(anm);
		anm->drop();
	}

	return node;
}


//! Adds a camera scene node which is able to be controlled with the mouse and keys
//! like in most first person shooters (FPS):
ICameraSceneNode* CSceneManager::addCameraSceneNodeFPS(ISceneNode* parent,
	f32 rotateSpeed, f32 moveSpeed, s32 id, SKeyMap* keyMapArray,
	s32 keyMapSize, bool noVerticalMovement, f32 jumpSpeed,
	bool invertMouseY, bool makeActive)
{
	ICameraSceneNode* node = addCameraSceneNode(parent, core::vector3df(),
			core::vector3df(0,0,100), id, makeActive);
	if (node)
	{
		ISceneNodeAnimator* anm = new CSceneNodeAnimatorCameraFPS(CursorControl,
				rotateSpeed, moveSpeed, jumpSpeed,
				keyMapArray, keyMapSize, noVerticalMovement, invertMouseY);

		// Bind the node's rotation to its target. This is consistent with 1.4.2 and below.
		node->bindTargetAndRotation(true);
		node->addAnimator(anm);
		anm->drop();
	}

	return node;
}


//! Adds a dynamic light scene node. The light will cast dynamic light on all
//! other scene nodes in the scene, which have the material flag video::MTF_LIGHTING
//! turned on. (This is the default setting in most scene nodes).
ILightSceneNode* CSceneManager::addLightSceneNode(ISceneNode* parent,
	const core::vector3df& position, video::SColorf color, f32 range, s32 id)
{
	if (!parent)
		parent = this;

	ILightSceneNode* node = new CLightSceneNode(parent, this, id, position, color, range);
	node->drop();

	return node;
}


//! Adds a billboard scene node to the scene. A billboard is like a 3d sprite: A 2d element,
//! which always looks to the camera. It is usually used for things like explosions, fire,
//! lensflares and things like that.
IBillboardSceneNode* CSceneManager::addBillboardSceneNode(ISceneNode* parent,
	const core::dimension2d<f32>& size, const core::vector3df& position, s32 id,
	video::SColor colorTop, video::SColor colorBottom
	)
{
	if (!parent)
		parent = this;

	IBillboardSceneNode* node = new CBillboardSceneNode(parent, this, id, position, size,
		colorTop, colorBottom);
	node->drop();

	return node;
}


//! Adds a skybox scene node. A skybox is a big cube with 6 textures on it and
//! is drawn around the camera position.
ISceneNode* CSceneManager::addSkyBoxSceneNode(video::ITexture* top, video::ITexture* bottom,
	video::ITexture* left, video::ITexture* right, video::ITexture* front,
	video::ITexture* back, ISceneNode* parent, s32 id)
{
	if (!parent)
		parent = this;

	ISceneNode* node = new CSkyBoxSceneNode(top, bottom, left, right,
			front, back, parent, this, id);

	node->drop();
	return node;
}


//! Adds a skydome scene node. A skydome is a large (half-) sphere with a
//! panoramic texture on it and is drawn around the camera position.
ISceneNode* CSceneManager::addSkyDomeSceneNode(video::ITexture* texture,
	u32 horiRes, u32 vertRes, f32 texturePercentage,f32 spherePercentage, f32 radius,
	ISceneNode* parent, s32 id)
{
	if (!parent)
		parent = this;

	ISceneNode* node = new CSkyDomeSceneNode(texture, horiRes, vertRes,
		texturePercentage, spherePercentage, radius, parent, this, id);

	node->drop();
	return node;
}


//! Adds a particle system scene node.
IParticleSystemSceneNode* CSceneManager::addParticleSystemSceneNode(
	bool withDefaultEmitter, ISceneNode* parent, s32 id,
	const core::vector3df& position, const core::vector3df& rotation,
	const core::vector3df& scale)
{
	if (!parent)
		parent = this;

	IParticleSystemSceneNode* node = new CParticleSystemSceneNode(withDefaultEmitter,
		parent, this, id, position, rotation, scale);
	node->drop();

	return node;
}


//! Adds a terrain scene node to the scene graph.
ITerrainSceneNode* CSceneManager::addTerrainSceneNode(
	const io::path& heightMapFileName,
	ISceneNode* parent, s32 id,
	const core::vector3df& position,
	const core::vector3df& rotation,
	const core::vector3df& scale,
	video::SColor vertexColor,
	s32 maxLOD, E_TERRAIN_PATCH_SIZE patchSize, s32 smoothFactor,
	bool addAlsoIfHeightmapEmpty)
{
	io::IReadFile* file = FileSystem->createAndOpenFile(heightMapFileName);

	if (!file && !addAlsoIfHeightmapEmpty)
	{
		os::Printer::log("Could not load terrain, because file could not be opened.",
		heightMapFileName, ELL_ERROR);
		return 0;
	}

	ITerrainSceneNode* terrain = addTerrainSceneNode(file, parent, id,
		position, rotation, scale, vertexColor, maxLOD, patchSize,
		smoothFactor, addAlsoIfHeightmapEmpty);

	if (file)
		file->drop();

	return terrain;
}

//! Adds a terrain scene node to the scene graph.
ITerrainSceneNode* CSceneManager::addTerrainSceneNode(
	io::IReadFile* heightMapFile,
	ISceneNode* parent, s32 id,
	const core::vector3df& position,
	const core::vector3df& rotation,
	const core::vector3df& scale,
	video::SColor vertexColor,
	s32 maxLOD, E_TERRAIN_PATCH_SIZE patchSize,
	s32 smoothFactor,
	bool addAlsoIfHeightmapEmpty)
{
	if (!parent)
		parent = this;

	if (!heightMapFile && !addAlsoIfHeightmapEmpty)
	{
		os::Printer::log("Could not load terrain, because file could not be opened.", ELL_ERROR);
		return 0;
	}

	CTerrainSceneNode* node = new CTerrainSceneNode(parent, this, FileSystem, id,
		maxLOD, patchSize, position, rotation, scale);

	if (!node->loadHeightMap(heightMapFile, vertexColor, smoothFactor))
	{
		if (!addAlsoIfHeightmapEmpty)
		{
			node->remove();
			node->drop();
			return 0;
		}
	}

	node->drop();
	return node;
}


//! Adds an empty scene node.
ISceneNode* CSceneManager::addEmptySceneNode(ISceneNode* parent, s32 id)
{
	if (!parent)
		parent = this;

	ISceneNode* node = new CEmptySceneNode(parent, this, id);
	node->drop();

	return node;
}


//! Adds a dummy transformation scene node to the scene graph.
IDummyTransformationSceneNode* CSceneManager::addDummyTransformationSceneNode(
	ISceneNode* parent, s32 id)
{
	if (!parent)
		parent = this;

	IDummyTransformationSceneNode* node = new CDummyTransformationSceneNode(
		parent, this, id);
	node->drop();

	return node;
}

//! Adds a Hill Plane mesh to the mesh pool. The mesh is generated on the fly
//! and looks like a plane with some hills on it. You can specify how many hills
//! there should be on the plane and how high they should be. Also you must
//! specify a name for the mesh, because the mesh is added to the mesh pool,
//! and can be retrieved again using ISceneManager::getMesh with the name as
//! parameter.
IAnimatedMesh* CSceneManager::addHillPlaneMesh(const io::path& name,
		const core::dimension2d<f32>& tileSize,
		const core::dimension2d<u32>& tileCount,
		video::SMaterial* material, f32 hillHeight,
		const core::dimension2d<f32>& countHills,
		const core::dimension2d<f32>& textureRepeatCount)
{
	if (MeshCache->isMeshLoaded(name))
		return MeshCache->getMeshByName(name);

	IMesh* mesh = GeometryCreator->createHillPlaneMesh(tileSize,
			tileCount, material, hillHeight, countHills,
			textureRepeatCount);
	if (!mesh)
		return 0;

	SAnimatedMesh* animatedMesh = new SAnimatedMesh();
	if (!animatedMesh)
	{
		mesh->drop();
		return 0;
	}

	animatedMesh->addMesh(mesh);
	mesh->drop();
	animatedMesh->recalculateBoundingBox();

	MeshCache->addMesh(name, animatedMesh);
	animatedMesh->drop();

	return animatedMesh;
}


//! Adds a terrain mesh to the mesh pool.
IAnimatedMesh* CSceneManager::addTerrainMesh(const io::path& name,
	video::IImage* texture, video::IImage* heightmap,
	const core::dimension2d<f32>& stretchSize,
	f32 maxHeight,
	const core::dimension2d<u32>& defaultVertexBlockSize)
{
	if (MeshCache->isMeshLoaded(name))
		return MeshCache->getMeshByName(name);

	const bool debugBorders=false;
	IMesh* mesh = GeometryCreator->createTerrainMesh(texture, heightmap,
			stretchSize, maxHeight, Driver,
			defaultVertexBlockSize, debugBorders);
	if (!mesh)
		return 0;

	SAnimatedMesh* animatedMesh = new SAnimatedMesh();
	if (!animatedMesh)
	{
		mesh->drop();
		return 0;
	}

	animatedMesh->addMesh(mesh);
	mesh->drop();
	animatedMesh->recalculateBoundingBox();

	MeshCache->addMesh(name, animatedMesh);
	animatedMesh->drop();

	return animatedMesh;
}


//! Adds an arrow mesh to the mesh pool.
IAnimatedMesh* CSceneManager::addArrowMesh(const io::path& name,
		video::SColor vtxColor0, video::SColor vtxColor1,
		u32 tesselationCylinder, u32 tesselationCone, f32 height,
		f32 cylinderHeight, f32 width0,f32 width1)
{
	if (MeshCache->isMeshLoaded(name))
		return MeshCache->getMeshByName(name);

	IMesh* mesh = GeometryCreator->createArrowMesh( tesselationCylinder,
			tesselationCone, height, cylinderHeight, width0,width1,
			vtxColor0, vtxColor1);
	if (!mesh)
		return 0;

	SAnimatedMesh* animatedMesh = new SAnimatedMesh();
	if (!animatedMesh)
	{
		mesh->drop();
		return 0;
	}

	animatedMesh->addMesh(mesh);
	mesh->drop();
	animatedMesh->recalculateBoundingBox();

	MeshCache->addMesh(name, animatedMesh);
	animatedMesh->drop();

	return animatedMesh;
}


//! Adds a static sphere mesh to the mesh pool.
IAnimatedMesh* CSceneManager::addSphereMesh(const io::path& name,
		f32 radius, u32 polyCountX, u32 polyCountY)
{
	if (MeshCache->isMeshLoaded(name))
		return MeshCache->getMeshByName(name);

	IMesh* mesh = GeometryCreator->createSphereMesh(radius, polyCountX, polyCountY);
	if (!mesh)
		return 0;

	SAnimatedMesh* animatedMesh = new SAnimatedMesh();
	if (!animatedMesh)
	{
		mesh->drop();
		return 0;
	}

	animatedMesh->addMesh(mesh);
	mesh->drop();
	animatedMesh->recalculateBoundingBox();

	MeshCache->addMesh(name, animatedMesh);
	animatedMesh->drop();

	return animatedMesh;
}



//! Adds a static volume light mesh to the mesh pool.
IAnimatedMesh* CSceneManager::addVolumeLightMesh(const io::path& name,
		const u32 SubdivideU, const u32 SubdivideV,
		const video::SColor FootColor, const video::SColor TailColor)
{
	if (MeshCache->isMeshLoaded(name))
		return MeshCache->getMeshByName(name);

	IMesh* mesh = GeometryCreator->createVolumeLightMesh(SubdivideU, SubdivideV, FootColor, TailColor);
	if (!mesh)
		return 0;

	SAnimatedMesh* animatedMesh = new SAnimatedMesh();
	if (!animatedMesh)
	{
		mesh->drop();
		return 0;
	}

	animatedMesh->addMesh(mesh);
	mesh->drop();
	animatedMesh->recalculateBoundingBox();

	MeshCache->addMesh(name, animatedMesh);
	animatedMesh->drop();

	return animatedMesh;
}


//! Returns the root scene node. This is the scene node wich is parent
//! of all scene nodes. The root scene node is a special scene node which
//! only exists to manage all scene nodes. It is not rendered and cannot
//! be removed from the scene.
//! \return Returns a pointer to the root scene node.
ISceneNode* CSceneManager::getRootSceneNode()
{
	return this;
}


//! Returns the current active camera.
//! \return The active camera is returned. Note that this can be NULL, if there
//! was no camera created yet.
ICameraSceneNode* CSceneManager::getActiveCamera() const
{
	return ActiveCamera;
}


//! Sets the active camera. The previous active camera will be deactivated.
//! \param camera: The new camera which should be active.
void CSceneManager::setActiveCamera(ICameraSceneNode* camera)
{
	if (camera)
		camera->grab();
	if (ActiveCamera)
		ActiveCamera->drop();

	ActiveCamera = camera;
}


//! renders the node.
void CSceneManager::render()
{
}


//! returns the axis aligned bounding box of this node
const core::aabbox3d<f32>& CSceneManager::getBoundingBox() const
{
	_IRR_DEBUG_BREAK_IF(true) // Bounding Box of Scene Manager wanted.

	// should never be used.
	return *((core::aabbox3d<f32>*)0);
}


//! returns if node is culled
bool CSceneManager::isCulled(const ISceneNode* node) const
{
	const ICameraSceneNode* cam = getActiveCamera();
	if (!cam)
	{
		_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
		return false;
	}
	bool result = false;

	// has occlusion query information
	if (node->getAutomaticCulling() & scene::EAC_OCC_QUERY)
	{
		result = (Driver->getOcclusionQueryResult(const_cast<ISceneNode*>(node))==0);
	}

	// can be seen by a bounding box ?
	if (!result && (node->getAutomaticCulling() & scene::EAC_BOX))
	{
		core::aabbox3d<f32> tbox = node->getBoundingBox();
		node->getAbsoluteTransformation().transformBoxEx(tbox);
		result = !(tbox.intersectsWithBox(cam->getViewFrustum()->getBoundingBox() ));
	}

	// can be seen by a bounding sphere
	if (!result && (node->getAutomaticCulling() & scene::EAC_FRUSTUM_SPHERE))
	{ // requires bbox diameter
	}

	// can be seen by cam pyramid planes ?
	if (!result && (node->getAutomaticCulling() & scene::EAC_FRUSTUM_BOX))
	{
		SViewFrustum frust = *cam->getViewFrustum();

		//transform the frustum to the node's current absolute transformation
		core::matrix4 invTrans(node->getAbsoluteTransformation(), core::matrix4::EM4CONST_INVERSE);
		//invTrans.makeInverse();
		frust.transform(invTrans);

		core::vector3df edges[8];
		node->getBoundingBox().getEdges(edges);

		for (s32 i=0; i<scene::SViewFrustum::VF_PLANE_COUNT; ++i)
		{
			bool boxInFrustum=false;
			for (u32 j=0; j<8; ++j)
			{
				if (frust.planes[i].classifyPointRelation(edges[j]) != core::ISREL3D_FRONT)
				{
					boxInFrustum=true;
					break;
				}
			}

			if (!boxInFrustum)
			{
				result = true;
				break;
			}
		}
	}

	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return result;
}


//! registers a node for rendering it at a specific time.
u32 CSceneManager::registerNodeForRendering(ISceneNode* node, E_SCENE_NODE_RENDER_PASS pass)
{
	u32 taken = 0;

	switch(pass)
	{
		// take camera if it is not already registered
	case ESNRP_CAMERA:
		{
			taken = 1;
			for (u32 i = 0; i != CameraList.size(); ++i)
			{
				if (CameraList[i] == node)
				{
					taken = 0;
					break;
				}
			}
			if (taken)
			{
				CameraList.push_back(node);
			}
		}
		break;

	case ESNRP_LIGHT:
		// TODO: Point Light culling..
		// Lighting model in irrlicht has to be redone..
		//if (!isCulled(node))
		{
			LightList.push_back(node);
			taken = 1;
		}
		break;

	case ESNRP_SKY_BOX:
		SkyBoxList.push_back(node);
		taken = 1;
		break;
	case ESNRP_SOLID:
		if (!isCulled(node))
		{
			SolidNodeList.push_back(node);
			taken = 1;
		}
		break;
	case ESNRP_TRANSPARENT:
		if (!isCulled(node))
		{
			TransparentNodeList.push_back(TransparentNodeEntry(node, camWorldPos));
			taken = 1;
		}
		break;
	case ESNRP_TRANSPARENT_EFFECT:
		if (!isCulled(node))
		{
			TransparentEffectNodeList.push_back(TransparentNodeEntry(node, camWorldPos));
			taken = 1;
		}
		break;
	case ESNRP_AUTOMATIC:
		if (!isCulled(node))
		{
			const u32 count = node->getMaterialCount();

			taken = 0;
			for (u32 i=0; i<count; ++i)
			{
				video::IMaterialRenderer* rnd =
					Driver->getMaterialRenderer(node->getMaterial(i).MaterialType);
				if (rnd && rnd->isTransparent())
				{
					// register as transparent node
					TransparentNodeEntry e(node, camWorldPos);
					TransparentNodeList.push_back(e);
					taken = 1;
					break;
				}
			}

			// not transparent, register as solid
			if (!taken)
			{
				SolidNodeList.push_back(node);
				taken = 1;
			}
		}
		break;
	case ESNRP_SHADOW:
		if (!isCulled(node))
		{
			ShadowNodeList.push_back(node);
			taken = 1;
		}
		break;

	case ESNRP_NONE: // ignore this one
		break;
	}

#ifdef _IRR_SCENEMANAGER_DEBUG
	s32 index = Parameters.findAttribute ( "calls" );
	Parameters.setAttribute ( index, Parameters.getAttributeAsInt ( index ) + 1 );

	if (!taken)
	{
		index = Parameters.findAttribute ( "culled" );
		Parameters.setAttribute ( index, Parameters.getAttributeAsInt ( index ) + 1 );
	}
#endif

	return taken;
}


//! This method is called just before the rendering process of the whole scene.
//! draws all scene nodes
void CSceneManager::drawAll()
{
	if (!Driver)
		return;

#ifdef _IRR_SCENEMANAGER_DEBUG
	// reset attributes
	Parameters.setAttribute ( "culled", 0 );
	Parameters.setAttribute ( "calls", 0 );
	Parameters.setAttribute ( "drawn_solid", 0 );
	Parameters.setAttribute ( "drawn_transparent", 0 );
	Parameters.setAttribute ( "drawn_transparent_effect", 0 );
#endif

	u32 i; // new ISO for scoping problem in some compilers

	// reset all transforms
	Driver->setMaterial(video::SMaterial());
	Driver->setTransform ( video::ETS_PROJECTION, core::IdentityMatrix );
	Driver->setTransform ( video::ETS_VIEW, core::IdentityMatrix );
	Driver->setTransform ( video::ETS_WORLD, core::IdentityMatrix );
	for (i=video::ETS_COUNT-1; i>=video::ETS_TEXTURE_0; --i)
		Driver->setTransform ( (video::E_TRANSFORMATION_STATE)i, core::IdentityMatrix );

	// TODO: This should not use an attribute here but a real parameter when necessary (too slow!)
	Driver->setAllowZWriteOnTransparent(Parameters.getAttributeAsBool( ALLOW_ZWRITE_ON_TRANSPARENT) );

	// do animations and other stuff.
	OnAnimate(os::Timer::getTime());

	/*!
		First Scene Node for prerendering should be the active camera
		consistent Camera is needed for culling
	*/
	camWorldPos.set(0,0,0);
	if (ActiveCamera)
	{
		ActiveCamera->render();
		camWorldPos = ActiveCamera->getAbsolutePosition();
	}

	// let all nodes register themselves
	OnRegisterSceneNode();

	if (LightManager)
		LightManager->OnPreRender(LightList);

	//render camera scenes
	{
		CurrentRendertime = ESNRP_CAMERA;
		Driver->getOverrideMaterial().Enabled = ((Driver->getOverrideMaterial().EnablePasses & CurrentRendertime) != 0);

		if (LightManager)
			LightManager->OnRenderPassPreRender(CurrentRendertime);

		for (i=0; i<CameraList.size(); ++i)
			CameraList[i]->render();

		CameraList.set_used(0);

		if (LightManager)
			LightManager->OnRenderPassPostRender(CurrentRendertime);
	}

	//render lights scenes
	{
		CurrentRendertime = ESNRP_LIGHT;
		Driver->getOverrideMaterial().Enabled = ((Driver->getOverrideMaterial().EnablePasses & CurrentRendertime) != 0);

		if (LightManager)
		{
			LightManager->OnRenderPassPreRender(CurrentRendertime);
		}
		else
		{
			// Sort the lights by distance from the camera
			core::vector3df camWorldPos(0, 0, 0);
			if (ActiveCamera)
				camWorldPos = ActiveCamera->getAbsolutePosition();

			core::array<DistanceNodeEntry> SortedLights;
			SortedLights.set_used(LightList.size());
			for (s32 light = (s32)LightList.size() - 1; light >= 0; --light)
				SortedLights[light].setNodeAndDistanceFromPosition(LightList[light], camWorldPos);

			SortedLights.set_sorted(false);
			SortedLights.sort();

			for(s32 light = (s32)LightList.size() - 1; light >= 0; --light)
				LightList[light] = SortedLights[light].Node;
		}

		Driver->deleteAllDynamicLights();

		Driver->setAmbientLight(AmbientLight);

		u32 maxLights = LightList.size();

		if (!LightManager)
			maxLights = core::min_ ( Driver->getMaximalDynamicLightAmount(), maxLights);

		for (i=0; i< maxLights; ++i)
			LightList[i]->render();

		if (LightManager)
			LightManager->OnRenderPassPostRender(CurrentRendertime);
	}

	// render skyboxes
	{
		CurrentRendertime = ESNRP_SKY_BOX;
		Driver->getOverrideMaterial().Enabled = ((Driver->getOverrideMaterial().EnablePasses & CurrentRendertime) != 0);

		if (LightManager)
		{
			LightManager->OnRenderPassPreRender(CurrentRendertime);
			for (i=0; i<SkyBoxList.size(); ++i)
			{
				ISceneNode* node = SkyBoxList[i];
				LightManager->OnNodePreRender(node);
				node->render();
				LightManager->OnNodePostRender(node);
			}
		}
		else
		{
			for (i=0; i<SkyBoxList.size(); ++i)
				SkyBoxList[i]->render();
		}

		SkyBoxList.set_used(0);

		if (LightManager)
			LightManager->OnRenderPassPostRender(CurrentRendertime);
	}


	// render default objects
	{
		CurrentRendertime = ESNRP_SOLID;
		Driver->getOverrideMaterial().Enabled = ((Driver->getOverrideMaterial().EnablePasses & CurrentRendertime) != 0);

		SolidNodeList.sort(); // sort by textures

		if (LightManager)
		{
			LightManager->OnRenderPassPreRender(CurrentRendertime);
			for (i=0; i<SolidNodeList.size(); ++i)
			{
				ISceneNode* node = SolidNodeList[i].Node;
				LightManager->OnNodePreRender(node);
				node->render();
				LightManager->OnNodePostRender(node);
			}
		}
		else
		{
			for (i=0; i<SolidNodeList.size(); ++i)
				SolidNodeList[i].Node->render();
		}

#ifdef _IRR_SCENEMANAGER_DEBUG
		Parameters.setAttribute("drawn_solid", (s32) SolidNodeList.size() );
#endif
		SolidNodeList.set_used(0);

		if (LightManager)
			LightManager->OnRenderPassPostRender(CurrentRendertime);
	}

	// render shadows
	{
		CurrentRendertime = ESNRP_SHADOW;
		Driver->getOverrideMaterial().Enabled = ((Driver->getOverrideMaterial().EnablePasses & CurrentRendertime) != 0);

		if (LightManager)
		{
			LightManager->OnRenderPassPreRender(CurrentRendertime);
			for (i=0; i<ShadowNodeList.size(); ++i)
			{
				ISceneNode* node = ShadowNodeList[i];
				LightManager->OnNodePreRender(node);
				node->render();
				LightManager->OnNodePostRender(node);
			}
		}
		else
		{
			for (i=0; i<ShadowNodeList.size(); ++i)
				ShadowNodeList[i]->render();
		}

		if (!ShadowNodeList.empty())
			Driver->drawStencilShadow(true,ShadowColor, ShadowColor,
				ShadowColor, ShadowColor);

		ShadowNodeList.set_used(0);

		if (LightManager)
			LightManager->OnRenderPassPostRender(CurrentRendertime);
	}

	// render transparent objects.
	{
		CurrentRendertime = ESNRP_TRANSPARENT;
		Driver->getOverrideMaterial().Enabled = ((Driver->getOverrideMaterial().EnablePasses & CurrentRendertime) != 0);

		TransparentNodeList.sort(); // sort by distance from camera
		if (LightManager)
		{
			LightManager->OnRenderPassPreRender(CurrentRendertime);

			for (i=0; i<TransparentNodeList.size(); ++i)
			{
				ISceneNode* node = TransparentNodeList[i].Node;
				LightManager->OnNodePreRender(node);
				node->render();
				LightManager->OnNodePostRender(node);
			}
		}
		else
		{
			for (i=0; i<TransparentNodeList.size(); ++i)
				TransparentNodeList[i].Node->render();
		}

#ifdef _IRR_SCENEMANAGER_DEBUG
		Parameters.setAttribute ( "drawn_transparent", (s32) TransparentNodeList.size() );
#endif
		TransparentNodeList.set_used(0);

		if (LightManager)
			LightManager->OnRenderPassPostRender(CurrentRendertime);
	}

	// render transparent effect objects.
	{
		CurrentRendertime = ESNRP_TRANSPARENT_EFFECT;
		Driver->getOverrideMaterial().Enabled = ((Driver->getOverrideMaterial().EnablePasses & CurrentRendertime) != 0);

		TransparentEffectNodeList.sort(); // sort by distance from camera

		if (LightManager)
		{
			LightManager->OnRenderPassPreRender(CurrentRendertime);

			for (i=0; i<TransparentEffectNodeList.size(); ++i)
			{
				ISceneNode* node = TransparentEffectNodeList[i].Node;
				LightManager->OnNodePreRender(node);
				node->render();
				LightManager->OnNodePostRender(node);
			}
		}
		else
		{
			for (i=0; i<TransparentEffectNodeList.size(); ++i)
				TransparentEffectNodeList[i].Node->render();
		}
#ifdef _IRR_SCENEMANAGER_DEBUG
		Parameters.setAttribute ( "drawn_transparent_effect", (s32) TransparentEffectNodeList.size() );
#endif
		TransparentEffectNodeList.set_used(0);
	}

	if (LightManager)
		LightManager->OnPostRender();

	LightList.set_used(0);
	clearDeletionList();

	CurrentRendertime = ESNRP_NONE;
}

void CSceneManager::setLightManager(ILightManager* lightManager)
{
	if (lightManager)
		lightManager->grab();
	if (LightManager)
		LightManager->drop();

	LightManager = lightManager;
}


//! Sets the color of stencil buffers shadows drawn by the scene manager.
void CSceneManager::setShadowColor(video::SColor color)
{
	ShadowColor = color;
}


//! Returns the current color of shadows.
video::SColor CSceneManager::getShadowColor() const
{
	return ShadowColor;
}


//! creates a rotation animator, which rotates the attached scene node around itself.
ISceneNodeAnimator* CSceneManager::createRotationAnimator(const core::vector3df& rotationPerSecond)
{
	ISceneNodeAnimator* anim = new CSceneNodeAnimatorRotation(os::Timer::getTime(),
		rotationPerSecond);

	return anim;
}


//! creates a fly circle animator, which lets the attached scene node fly around a center.
ISceneNodeAnimator* CSceneManager::createFlyCircleAnimator(
		const core::vector3df& center, f32 radius, f32 speed,
		const core::vector3df& direction,
		f32 startPosition,
		f32 radiusEllipsoid)
{
	const f32 orbitDurationMs = (core::DEGTORAD * 360.f) / speed;
	const u32 effectiveTime = os::Timer::getTime() + (u32)(orbitDurationMs * startPosition);

	ISceneNodeAnimator* anim = new CSceneNodeAnimatorFlyCircle(
			effectiveTime, center,
			radius, speed, direction,radiusEllipsoid);
	return anim;
}


//! Creates a fly straight animator, which lets the attached scene node
//! fly or move along a line between two points.
ISceneNodeAnimator* CSceneManager::createFlyStraightAnimator(const core::vector3df& startPoint,
					const core::vector3df& endPoint, u32 timeForWay, bool loop,bool pingpong)
{
	ISceneNodeAnimator* anim = new CSceneNodeAnimatorFlyStraight(startPoint,
		endPoint, timeForWay, loop, os::Timer::getTime(), pingpong);

	return anim;
}


//! Creates a texture animator, which switches the textures of the target scene
//! node based on a list of textures.
ISceneNodeAnimator* CSceneManager::createTextureAnimator(const core::array<video::ITexture*>& textures,
	s32 timePerFrame, bool loop)
{
	ISceneNodeAnimator* anim = new CSceneNodeAnimatorTexture(textures,
		timePerFrame, loop, os::Timer::getTime());

	return anim;
}


//! Creates a scene node animator, which deletes the scene node after
//! some time automaticly.
ISceneNodeAnimator* CSceneManager::createDeleteAnimator(u32 when)
{
	return new CSceneNodeAnimatorDelete(this, os::Timer::getTime() + when);
}


//! Creates a special scene node animator for doing automatic collision detection
//! and response.
ISceneNodeAnimatorCollisionResponse* CSceneManager::createCollisionResponseAnimator(
	ITriangleSelector* world, ISceneNode* sceneNode, const core::vector3df& ellipsoidRadius,
	const core::vector3df& gravityPerSecond,
	const core::vector3df& ellipsoidTranslation, f32 slidingValue)
{
	ISceneNodeAnimatorCollisionResponse* anim = new
		CSceneNodeAnimatorCollisionResponse(this, world, sceneNode,
			ellipsoidRadius, gravityPerSecond,
			ellipsoidTranslation, slidingValue);

	return anim;
}


//! Creates a follow spline animator.
ISceneNodeAnimator* CSceneManager::createFollowSplineAnimator(s32 startTime,
	const core::array< core::vector3df >& points,
	f32 speed, f32 tightness, bool loop, bool pingpong)
{
	ISceneNodeAnimator* a = new CSceneNodeAnimatorFollowSpline(startTime, points,
		speed, tightness, loop, pingpong);
	return a;
}


//! Adds an external mesh loader.
void CSceneManager::addExternalMeshLoader(IMeshLoader* externalLoader)
{
	if (!externalLoader)
		return;

	externalLoader->grab();
	MeshLoaderList.push_back(externalLoader);
}


//! Returns the number of mesh loaders supported by Irrlicht at this time
u32 CSceneManager::getMeshLoaderCount() const
{
	return MeshLoaderList.size();
}


//! Retrieve the given mesh loader
IMeshLoader* CSceneManager::getMeshLoader(u32 index) const
{
	if (index < MeshLoaderList.size())
		return MeshLoaderList[index];
	else
		return 0;
}


//! Adds an external scene loader.
void CSceneManager::addExternalSceneLoader(ISceneLoader* externalLoader)
{
	if (!externalLoader)
		return;

	externalLoader->grab();
	SceneLoaderList.push_back(externalLoader);
}


//! Returns the number of scene loaders
u32 CSceneManager::getSceneLoaderCount() const
{
	return SceneLoaderList.size();
}


//! Retrieve the given scene loader
ISceneLoader* CSceneManager::getSceneLoader(u32 index) const
{
	if (index < SceneLoaderList.size())
		return SceneLoaderList[index];
	else
		return 0;
}


//! Returns a pointer to the scene collision manager.
ISceneCollisionManager* CSceneManager::getSceneCollisionManager()
{
	return CollisionManager;
}


//! Returns a pointer to the mesh manipulator.
IMeshManipulator* CSceneManager::getMeshManipulator()
{
	return Driver->getMeshManipulator();
}


//! Creates a simple ITriangleSelector, based on a mesh.
ITriangleSelector* CSceneManager::createTriangleSelector(IMesh* mesh, ISceneNode* node)
{
	if (!mesh)
		return 0;

	return new CTriangleSelector(mesh, node);
}


//! Creates a simple and updatable ITriangleSelector, based on a the mesh owned by an
//! animated scene node
ITriangleSelector* CSceneManager::createTriangleSelector(IAnimatedMeshSceneNode* node)
{
	if (!node || !node->getMesh())
		return 0;

	return new CTriangleSelector(node);
}


//! Creates a simple dynamic ITriangleSelector, based on a axis aligned bounding box.
ITriangleSelector* CSceneManager::createTriangleSelectorFromBoundingBox(ISceneNode* node)
{
	if (!node)
		return 0;

	return new CTriangleBBSelector(node);
}


//! Creates a simple ITriangleSelector, based on a mesh.
ITriangleSelector* CSceneManager::createOctreeTriangleSelector(IMesh* mesh,
							ISceneNode* node, s32 minimalPolysPerNode)
{
	if (!mesh)
		return 0;

	return new COctreeTriangleSelector(mesh, node, minimalPolysPerNode);
}


//! Creates a meta triangle selector.
IMetaTriangleSelector* CSceneManager::createMetaTriangleSelector()
{
	return new CMetaTriangleSelector();
}


//! Creates a triangle selector which can select triangles from a terrain scene node
ITriangleSelector* CSceneManager::createTerrainTriangleSelector(
	ITerrainSceneNode* node, s32 LOD)
{
	return new CTerrainTriangleSelector(node, LOD);
}



//! Adds a scene node to the deletion queue.
void CSceneManager::addToDeletionQueue(ISceneNode* node)
{
	if (!node)
		return;

	node->grab();
	DeletionList.push_back(node);
}


//! clears the deletion list
void CSceneManager::clearDeletionList()
{
	if (DeletionList.empty())
		return;

	for (u32 i=0; i<DeletionList.size(); ++i)
	{
		DeletionList[i]->remove();
		DeletionList[i]->drop();
	}

	DeletionList.clear();
}


//! Returns the first scene node with the specified name.
ISceneNode* CSceneManager::getSceneNodeFromName(const char* name, ISceneNode* start)
{
	if (start == 0)
		start = getRootSceneNode();

	if (!strcmp(start->getName(),name))
		return start;

	ISceneNode* node = 0;

	const ISceneNodeList& list = start->getChildren();
	ISceneNodeList::ConstIterator it = list.begin();
	for (; it!=list.end(); ++it)
	{
		node = getSceneNodeFromName(name, *it);
		if (node)
			return node;
	}

	return 0;
}


//! Returns the first scene node with the specified id.
ISceneNode* CSceneManager::getSceneNodeFromId(s32 id, ISceneNode* start)
{
	if (start == 0)
		start = getRootSceneNode();

	if (start->getID() == id)
		return start;

	ISceneNode* node = 0;

	const ISceneNodeList& list = start->getChildren();
	ISceneNodeList::ConstIterator it = list.begin();
	for (; it!=list.end(); ++it)
	{
		node = getSceneNodeFromId(id, *it);
		if (node)
			return node;
	}

	return 0;
}


//! Returns the first scene node with the specified type.
ISceneNode* CSceneManager::getSceneNodeFromType(scene::ESCENE_NODE_TYPE type, ISceneNode* start)
{
	if (start == 0)
		start = getRootSceneNode();

	if (start->getType() == type || ESNT_ANY == type)
		return start;

	ISceneNode* node = 0;

	const ISceneNodeList& list = start->getChildren();
	ISceneNodeList::ConstIterator it = list.begin();
	for (; it!=list.end(); ++it)
	{
		node = getSceneNodeFromType(type, *it);
		if (node)
			return node;
	}

	return 0;
}


//! returns scene nodes by type.
void CSceneManager::getSceneNodesFromType(ESCENE_NODE_TYPE type, core::array<scene::ISceneNode*>& outNodes, ISceneNode* start)
{
	if (start == 0)
		start = getRootSceneNode();

	if (start->getType() == type || ESNT_ANY == type)
		outNodes.push_back(start);

	const ISceneNodeList& list = start->getChildren();
	ISceneNodeList::ConstIterator it = list.begin();

	for (; it!=list.end(); ++it)
	{
		getSceneNodesFromType(type, outNodes, *it);
	}
}


//! Posts an input event to the environment. Usually you do not have to
//! use this method, it is used by the internal engine.
bool CSceneManager::postEventFromUser(const SEvent& event)
{
	bool ret = false;
	ICameraSceneNode* cam = getActiveCamera();
	if (cam)
		ret = cam->OnEvent(event);

	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return ret;
}


//! Removes all children of this scene node
void CSceneManager::removeAll()
{
	ISceneNode::removeAll();
	setActiveCamera(0);
	// Make sure the driver is reset, might need a more complex method at some point
	if (Driver)
		Driver->setMaterial(video::SMaterial());
}


//! Clears the whole scene. All scene nodes are removed.
void CSceneManager::clear()
{
	removeAll();
}


//! Returns interface to the parameters set in this scene.
io::IAttributes* CSceneManager::getParameters()
{
	return &Parameters;
}


//! Returns current render pass.
E_SCENE_NODE_RENDER_PASS CSceneManager::getSceneNodeRenderPass() const
{
	return CurrentRendertime;
}


//! Returns an interface to the mesh cache which is shared beween all existing scene managers.
IMeshCache* CSceneManager::getMeshCache()
{
	return MeshCache;
}


//! Creates a new scene manager.
ISceneManager* CSceneManager::createNewSceneManager(bool cloneContent)
{
	CSceneManager* manager = new CSceneManager(Driver, FileSystem, CursorControl, MeshCache, GUIEnvironment);

	if (cloneContent)
		manager->cloneMembers(this, manager);

	return manager;
}


//! Returns the default scene node factory which can create all built in scene nodes
ISceneNodeFactory* CSceneManager::getDefaultSceneNodeFactory()
{
	return getSceneNodeFactory(0);
}


//! Adds a scene node factory to the scene manager.
void CSceneManager::registerSceneNodeFactory(ISceneNodeFactory* factoryToAdd)
{
	if (factoryToAdd)
	{
		factoryToAdd->grab();
		SceneNodeFactoryList.push_back(factoryToAdd);
	}
}


//! Returns amount of registered scene node factories.
u32 CSceneManager::getRegisteredSceneNodeFactoryCount() const
{
	return SceneNodeFactoryList.size();
}


//! Returns a scene node factory by index
ISceneNodeFactory* CSceneManager::getSceneNodeFactory(u32 index)
{
	if (index < SceneNodeFactoryList.size())
		return SceneNodeFactoryList[index];

	return 0;
}


//! Returns the default scene node animator factory which can create all built-in scene node animators
ISceneNodeAnimatorFactory* CSceneManager::getDefaultSceneNodeAnimatorFactory()
{
	return getSceneNodeAnimatorFactory(0);
}

//! Adds a scene node animator factory to the scene manager.
void CSceneManager::registerSceneNodeAnimatorFactory(ISceneNodeAnimatorFactory* factoryToAdd)
{
	if (factoryToAdd)
	{
		factoryToAdd->grab();
		SceneNodeAnimatorFactoryList.push_back(factoryToAdd);
	}
}


//! Returns amount of registered scene node animator factories.
u32 CSceneManager::getRegisteredSceneNodeAnimatorFactoryCount() const
{
	return SceneNodeAnimatorFactoryList.size();
}


//! Returns a scene node animator factory by index
ISceneNodeAnimatorFactory* CSceneManager::getSceneNodeAnimatorFactory(u32 index)
{
	if (index < SceneNodeAnimatorFactoryList.size())
		return SceneNodeAnimatorFactoryList[index];

	return 0;
}


//! Saves the current scene into a file.
//! \param filename: Name of the file .
bool CSceneManager::saveScene(const io::path& filename, ISceneUserDataSerializer* userDataSerializer, ISceneNode* node)
{
	bool ret = false;
	io::IWriteFile* file = FileSystem->createAndWriteFile(filename);
	if (file)
	{
		ret = saveScene(file, userDataSerializer, node);
		file->drop();
	}
	else
		os::Printer::log("Unable to open file", filename, ELL_ERROR);

	_IRR_IMPLEMENT_MANAGED_MARSHALLING_BUGFIX;
	return ret;
}


//! Saves the current scene into a file.
bool CSceneManager::saveScene(io::IWriteFile* file, ISceneUserDataSerializer* userDataSerializer, ISceneNode* node)
{
	if (!file)
	{
		return false;
	}

	bool result=false;
	io::IXMLWriter* writer = FileSystem->createXMLWriter(file);
	if (!writer)
	{
		os::Printer::log("Unable to create XML writer", file->getFileName(), ELL_ERROR);
	}
	else
	{
		result = saveScene(writer, FileSystem->getFileDir(FileSystem->getAbsolutePath(file->getFileName())), userDataSerializer, node);
		writer->drop();
	}
	return result;
}


//! Saves the current scene into a file.
bool CSceneManager::saveScene(io::IXMLWriter* writer, const io::path& currentPath, ISceneUserDataSerializer* userDataSerializer, ISceneNode* node)
{
	if (!writer)
		return false;

	if (!node)
		node=this;

	writer->writeXMLHeader();
	writeSceneNode(writer, node, userDataSerializer, currentPath.c_str(), true);

	return true;
}


//! Loads a scene.
bool CSceneManager::loadScene(const io::path& filename, ISceneUserDataSerializer* userDataSerializer, ISceneNode* rootNode)
{
	io::IReadFile* file = FileSystem->createAndOpenFile(filename);
	if (!file)
	{
		os::Printer::log("Unable to open scene file", filename.c_str(), ELL_ERROR);
		return false;
	}

	const bool ret = loadScene(file, userDataSerializer, rootNode);
	file->drop();

	return ret;
}


//! Loads a scene. Note that the current scene is not cleared before.
bool CSceneManager::loadScene(io::IReadFile* file, ISceneUserDataSerializer* userDataSerializer, ISceneNode* rootNode)
{
	if (!file)
	{
		os::Printer::log("Unable to open scene file", ELL_ERROR);
		return false;
	}

	bool ret = false;

	// try scene loaders in reverse order
	s32 i = SceneLoaderList.size()-1;
	for (; i >= 0 && !ret; --i)
		if (SceneLoaderList[i]->isALoadableFileFormat(file))
			ret = SceneLoaderList[i]->loadScene(file, userDataSerializer, rootNode);

	if (!ret)
		os::Printer::log("Could not load scene file, perhaps the format is unsupported: ", file->getFileName().c_str(), ELL_ERROR);

	return ret;
}


//! writes a scene node
void CSceneManager::writeSceneNode(io::IXMLWriter* writer, ISceneNode* node, ISceneUserDataSerializer* userDataSerializer,
		const fschar_t* currentPath, bool init)
{
	if (!writer || !node || node->isDebugObject())
		return;

	const wchar_t* name;
	ISceneNode* tmpNode=node;

	if (init)
	{
		name = IRR_XML_FORMAT_SCENE.c_str();
		writer->writeElement(name, false);
		node=this;
	}
	else
	{
		name = IRR_XML_FORMAT_NODE.c_str();
		writer->writeElement(name, false, IRR_XML_FORMAT_NODE_ATTR_TYPE.c_str(),
			core::stringw(getSceneNodeTypeName(node->getType())).c_str());
	}

	writer->writeLineBreak();

	// write properties

	io::IAttributes* attr = FileSystem->createEmptyAttributes(Driver);
	io::SAttributeReadWriteOptions options;
	if (currentPath)
	{
		options.Filename=currentPath;
		options.Flags|=io::EARWF_USE_RELATIVE_PATHS;
	}
	node->serializeAttributes(attr, &options);

	if (attr->getAttributeCount() != 0)
	{
		attr->write(writer);
		writer->writeLineBreak();
	}

	// write materials

	if (node->getMaterialCount() && Driver)
	{
		const wchar_t* materialElement = L"materials";

		writer->writeElement(materialElement);
		writer->writeLineBreak();

		for (u32 i=0; i < node->getMaterialCount(); ++i)
		{
			io::IAttributes* tmp_attr =
				Driver->createAttributesFromMaterial(node->getMaterial(i), &options);
			tmp_attr->write(writer);
			tmp_attr->drop();
		}

		writer->writeClosingTag(materialElement);
		writer->writeLineBreak();
	}

	// write animators

	if (!node->getAnimators().empty())
	{
		const wchar_t* animatorElement = L"animators";
		writer->writeElement(animatorElement);
		writer->writeLineBreak();

		ISceneNodeAnimatorList::ConstIterator it = node->getAnimators().begin();
		for (; it != node->getAnimators().end(); ++it)
		{
			attr->clear();
			attr->addString("Type", getAnimatorTypeName((*it)->getType()));

			(*it)->serializeAttributes(attr);

			attr->write(writer);
		}

		writer->writeClosingTag(animatorElement);
		writer->writeLineBreak();
	}

	// write possible user data

	if (userDataSerializer)
	{
		io::IAttributes* userData = userDataSerializer->createUserData(node);
		if (userData)
		{
			const wchar_t* userDataElement = L"userData";

			writer->writeLineBreak();
			writer->writeElement(userDataElement);
			writer->writeLineBreak();

			userData->write(writer);

			writer->writeClosingTag(userDataElement);
			writer->writeLineBreak();
			writer->writeLineBreak();

			userData->drop();
		}
	}
	// reset to actual root node
	if (init)
		node=tmpNode;

	// write children once root node is written
	// if parent is not scene manager, we need to write out node first
	if (init && (node != this))
	{
		writeSceneNode(writer, node, userDataSerializer, currentPath);
	}
	else
	{
		ISceneNodeList::ConstIterator it = node->getChildren().begin();
		for (; it != node->getChildren().end(); ++it)
			writeSceneNode(writer, (*it), userDataSerializer, currentPath);
	}

	attr->drop();

	writer->writeClosingTag(name);
	writer->writeLineBreak();
	writer->writeLineBreak();
}


//! Returns a typename from a scene node type or null if not found
const c8* CSceneManager::getSceneNodeTypeName(ESCENE_NODE_TYPE type)
{
	const char* name = 0;

	for (s32 i=(s32)SceneNodeFactoryList.size()-1; !name && i>=0; --i)
		name = SceneNodeFactoryList[i]->getCreateableSceneNodeTypeName(type);

	return name;
}

//! Adds a scene node to the scene by name
ISceneNode* CSceneManager::addSceneNode(const char* sceneNodeTypeName, ISceneNode* parent)
{
	ISceneNode* node = 0;

	for (s32 i=(s32)SceneNodeFactoryList.size()-1; i>=0 && !node; --i)
			node = SceneNodeFactoryList[i]->addSceneNode(sceneNodeTypeName, parent);

	return node;
}

ISceneNodeAnimator* CSceneManager::createSceneNodeAnimator(const char* typeName, ISceneNode* target)
{
	ISceneNodeAnimator *animator = 0;

	for (s32 i=(s32)SceneNodeAnimatorFactoryList.size()-1; i>=0 && !animator; --i)
		animator = SceneNodeAnimatorFactoryList[i]->createSceneNodeAnimator(typeName, target);

	return animator;
}


//! Returns a typename from a scene node animator type or null if not found
const c8* CSceneManager::getAnimatorTypeName(ESCENE_NODE_ANIMATOR_TYPE type)
{
	const char* name = 0;

	for (s32 i=SceneNodeAnimatorFactoryList.size()-1; !name && i >= 0; --i)
		name = SceneNodeAnimatorFactoryList[i]->getCreateableSceneNodeAnimatorTypeName(type);

	return name;
}


//! Writes attributes of the scene node.
void CSceneManager::serializeAttributes(io::IAttributes* out, io::SAttributeReadWriteOptions* options) const
{
	out->addString	("Name", Name.c_str());
	out->addInt	("Id", ID );
	out->addColorf	("AmbientLight", AmbientLight);

	// fog attributes from video driver
	video::SColor color;
	video::E_FOG_TYPE fogType;
	f32 start, end, density;
	bool pixelFog, rangeFog;

	Driver->getFog(color, fogType, start, end, density, pixelFog, rangeFog);

	out->addEnum("FogType", fogType, video::FogTypeNames);
	out->addColorf("FogColor", color);
	out->addFloat("FogStart", start);
	out->addFloat("FogEnd", end);
	out->addFloat("FogDensity", density);
	out->addBool("FogPixel", pixelFog);
	out->addBool("FogRange", rangeFog);
}

//! Reads attributes of the scene node.
void CSceneManager::deserializeAttributes(io::IAttributes* in, io::SAttributeReadWriteOptions* options)
{
	Name = in->getAttributeAsString("Name");
	ID = in->getAttributeAsInt("Id");
	AmbientLight = in->getAttributeAsColorf("AmbientLight");

	// fog attributes
	video::SColor color;
	video::E_FOG_TYPE fogType;
	f32 start, end, density;
	bool pixelFog, rangeFog;
	if (in->existsAttribute("FogType"))
	{
		fogType = (video::E_FOG_TYPE) in->getAttributeAsEnumeration("FogType", video::FogTypeNames);
		color = in->getAttributeAsColorf("FogColor").toSColor();
		start = in->getAttributeAsFloat("FogStart");
		end = in->getAttributeAsFloat("FogEnd");
		density = in->getAttributeAsFloat("FogDensity");
		pixelFog = in->getAttributeAsBool("FogPixel");
		rangeFog = in->getAttributeAsBool("FogRange");
		Driver->setFog(color, fogType, start, end, density, pixelFog, rangeFog);
	}

	RelativeTranslation.set(0,0,0);
	RelativeRotation.set(0,0,0);
	RelativeScale.set(1,1,1);
	IsVisible = true;
	AutomaticCullingState = scene::EAC_BOX;
	DebugDataVisible = scene::EDS_OFF;
	IsDebugObject = false;

	updateAbsolutePosition();
}


//! Sets ambient color of the scene
void CSceneManager::setAmbientLight(const video::SColorf &ambientColor)
{
	AmbientLight = ambientColor;
}


//! Returns ambient color of the scene
const video::SColorf& CSceneManager::getAmbientLight() const
{
	return AmbientLight;
}


//! Get a skinned mesh, which is not available as header-only code
ISkinnedMesh* CSceneManager::createSkinnedMesh()
{
#ifdef _IRR_COMPILE_WITH_SKINNED_MESH_SUPPORT_
	return new CSkinnedMesh();
#else
	return 0;
#endif
}

//! Returns a mesh writer implementation if available
IMeshWriter* CSceneManager::createMeshWriter(EMESH_WRITER_TYPE type)
{
	switch(type)
	{
	case EMWT_IRR_MESH:
#ifdef _IRR_COMPILE_WITH_IRR_WRITER_
		return new CIrrMeshWriter(Driver, FileSystem);
#else
		return 0;
#endif
	case EMWT_COLLADA:
#ifdef _IRR_COMPILE_WITH_COLLADA_WRITER_
		return new CColladaMeshWriter(this, Driver, FileSystem);
#else
		return 0;
#endif
	case EMWT_STL:
#ifdef _IRR_COMPILE_WITH_STL_WRITER_
		return new CSTLMeshWriter(this);
#else
		return 0;
#endif
	case EMWT_OBJ:
#ifdef _IRR_COMPILE_WITH_OBJ_WRITER_
		return new COBJMeshWriter(this, FileSystem);
#else
		return 0;
#endif

	case EMWT_PLY:
#ifdef _IRR_COMPILE_WITH_PLY_WRITER_
		return new CPLYMeshWriter();
#else
		return 0;
#endif
	}

	return 0;
}


// creates a scenemanager
ISceneManager* createSceneManager(video::IVideoDriver* driver,
		io::IFileSystem* fs, gui::ICursorControl* cursorcontrol,
		gui::IGUIEnvironment *guiEnvironment)
{
	return new CSceneManager(driver, fs, cursorcontrol, 0, guiEnvironment );
}


} // end namespace scene
} // end namespace irr

