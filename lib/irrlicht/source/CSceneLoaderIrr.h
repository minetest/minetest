// Copyright (C) 2010-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_SCENE_LOADER_IRR_H_INCLUDED__
#define __C_SCENE_LOADER_IRR_H_INCLUDED__

#include "ISceneLoader.h"

#include "IXMLReader.h"

namespace irr
{

namespace io
{
	class IFileSystem;
}

namespace scene
{

class ISceneManager;

//! Class which can load a scene into the scene manager.
class CSceneLoaderIrr : public virtual ISceneLoader
{
public:

	//! Constructor
	CSceneLoaderIrr(ISceneManager *smgr, io::IFileSystem* fs);

	//! Destructor
	virtual ~CSceneLoaderIrr();

	//! Returns true if the class might be able to load this file.
	virtual bool isALoadableFileExtension(const io::path& filename) const;

	//! Returns true if the class might be able to load this file.
	virtual bool isALoadableFileFormat(io::IReadFile *file) const;

	//! Loads the scene into the scene manager.
	virtual bool loadScene(io::IReadFile* file, ISceneUserDataSerializer* userDataSerializer=0,
	                       ISceneNode* rootNode=0);

private:

	//! Recursively reads nodes from the xml file
	void readSceneNode(io::IXMLReader* reader, ISceneNode* parent,
		ISceneUserDataSerializer* userDataSerializer);

	//! read a node's materials
	void readMaterials(io::IXMLReader* reader, ISceneNode* node);

	//! read a node's animators
	void readAnimators(io::IXMLReader* reader, ISceneNode* node);

	//! read any other data into the user serializer
	void readUserData(io::IXMLReader* reader, ISceneNode* node,
		ISceneUserDataSerializer* userDataSerializer);

	ISceneManager   *SceneManager;
	io::IFileSystem *FileSystem;

	//! constants for reading and writing XML.
	//! Not made static due to portability problems.
	// TODO: move to own header
	const core::stringw IRR_XML_FORMAT_SCENE;
	const core::stringw IRR_XML_FORMAT_NODE;
	const core::stringw IRR_XML_FORMAT_NODE_ATTR_TYPE;
	const core::stringw IRR_XML_FORMAT_ATTRIBUTES;
	const core::stringw IRR_XML_FORMAT_MATERIALS;
	const core::stringw IRR_XML_FORMAT_ANIMATORS;
	const core::stringw IRR_XML_FORMAT_USERDATA;
};


} // end namespace scene
} // end namespace irr

#endif

