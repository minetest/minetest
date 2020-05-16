// Copyright (C) 2010-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_SCENE_LOADER_H_INCLUDED__
#define __I_SCENE_LOADER_H_INCLUDED__

#include "IReferenceCounted.h"
#include "path.h"

namespace irr
{
namespace io
{
	class IReadFile;
} // end namespace io
namespace scene
{
	class ISceneNode;
	class ISceneUserDataSerializer;

//! Class which can load a scene into the scene manager.
/** If you want Irrlicht to be able to load currently unsupported
scene file formats (e.g. .vrml), then implement this and add your
new Sceneloader to the engine with ISceneManager::addExternalSceneLoader(). */
class ISceneLoader : public virtual IReferenceCounted
{
public:

	//! Returns true if the class might be able to load this file.
	/** This decision should be based on the file extension (e.g. ".vrml")
	only.
	\param filename Name of the file to test.
	\return True if the extension is a recognised type. */
	virtual bool isALoadableFileExtension(const io::path& filename) const = 0;

	//! Returns true if the class might be able to load this file.
	/** This decision will be based on a quick look at the contents of the file.
	\param file The file to test.
	\return True if the extension is a recognised type. */
	virtual bool isALoadableFileFormat(io::IReadFile* file) const = 0;

	//! Loads the scene into the scene manager.
	/** \param file File which contains the scene.
	\param userDataSerializer: If you want to load user data which may be attached
	to some some scene nodes in the file, implement the ISceneUserDataSerializer
	interface and provide it as parameter here. Otherwise, simply specify 0 as this
	parameter.
	\param rootNode The node to load the scene into, if none is provided then the
	scene will be loaded into the root node.
	\return Returns true on success, false on failure. Returns 0 if loading failed. */
	virtual bool loadScene(io::IReadFile* file, ISceneUserDataSerializer* userDataSerializer=0,
	                       ISceneNode* rootNode=0) = 0;

};


} // end namespace scene
} // end namespace irr

#endif

