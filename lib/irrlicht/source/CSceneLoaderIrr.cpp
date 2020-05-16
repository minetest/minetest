// Copyright (C) 2010-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#include "CSceneLoaderIrr.h"
#include "ISceneNodeAnimatorFactory.h"
#include "ISceneUserDataSerializer.h"
#include "ISceneManager.h"
#include "IVideoDriver.h"
#include "IFileSystem.h"
#include "os.h"

namespace irr
{
namespace scene
{

//! Constructor
CSceneLoaderIrr::CSceneLoaderIrr(ISceneManager *smgr, io::IFileSystem* fs)
 : SceneManager(smgr), FileSystem(fs),
   IRR_XML_FORMAT_SCENE(L"irr_scene"), IRR_XML_FORMAT_NODE(L"node"), IRR_XML_FORMAT_NODE_ATTR_TYPE(L"type"),
   IRR_XML_FORMAT_ATTRIBUTES(L"attributes"), IRR_XML_FORMAT_MATERIALS(L"materials"),
   IRR_XML_FORMAT_ANIMATORS(L"animators"), IRR_XML_FORMAT_USERDATA(L"userData")
{

}

//! Destructor
CSceneLoaderIrr::~CSceneLoaderIrr()
{

}

//! Returns true if the class might be able to load this file.
bool CSceneLoaderIrr::isALoadableFileExtension(const io::path& filename) const
{
	return core::hasFileExtension(filename, "irr");
}

//! Returns true if the class might be able to load this file.
bool CSceneLoaderIrr::isALoadableFileFormat(io::IReadFile *file) const
{
	// todo: check inside the file
	return true;
}

//! Loads the scene into the scene manager.
bool CSceneLoaderIrr::loadScene(io::IReadFile* file, ISceneUserDataSerializer* userDataSerializer,
	ISceneNode* rootNode)
{
	if (!file)
	{
		os::Printer::log("Unable to open scene file", ELL_ERROR);
		return false;
	}

	io::IXMLReader* reader = FileSystem->createXMLReader(file);
	if (!reader)
	{
		os::Printer::log("Scene is not a valid XML file", file->getFileName().c_str(), ELL_ERROR);
		return false;
	}

	// TODO: COLLADA_CREATE_SCENE_INSTANCES can be removed when the COLLADA loader is a scene loader
	bool oldColladaSingleMesh = SceneManager->getParameters()->getAttributeAsBool(COLLADA_CREATE_SCENE_INSTANCES);
	SceneManager->getParameters()->setAttribute(COLLADA_CREATE_SCENE_INSTANCES, false);

	// read file
	while (reader->read())
	{
		readSceneNode(reader, rootNode, userDataSerializer);
	}

	// restore old collada parameters
	SceneManager->getParameters()->setAttribute(COLLADA_CREATE_SCENE_INSTANCES, oldColladaSingleMesh);

	// clean up
	reader->drop();
	return true;
}


//! Reads the next node
void CSceneLoaderIrr::readSceneNode(io::IXMLReader* reader, ISceneNode* parent,
	ISceneUserDataSerializer* userDataSerializer)
{
	if (!reader)
		return;

	scene::ISceneNode* node = 0;

	if (!parent && IRR_XML_FORMAT_SCENE==reader->getNodeName())
		node = SceneManager->getRootSceneNode();
	else if (parent && IRR_XML_FORMAT_NODE==reader->getNodeName())
	{
		// find node type and create it
		core::stringc attrName = reader->getAttributeValue(IRR_XML_FORMAT_NODE_ATTR_TYPE.c_str());

		node = SceneManager->addSceneNode(attrName.c_str(), parent);

		if (!node)
			os::Printer::log("Could not create scene node of unknown type", attrName.c_str());
	}
	else
		node=parent;

	// read attributes
	while(reader->read())
	{
		bool endreached = false;

		const wchar_t* name = reader->getNodeName();

		switch (reader->getNodeType())
		{
		case io::EXN_ELEMENT_END:
			if ((IRR_XML_FORMAT_NODE  == name) ||
				(IRR_XML_FORMAT_SCENE == name))
			{
				endreached = true;
			}
			break;
		case io::EXN_ELEMENT:
			if (IRR_XML_FORMAT_ATTRIBUTES == name)
			{
				// read attributes
				io::IAttributes* attr = FileSystem->createEmptyAttributes(SceneManager->getVideoDriver());
				attr->read(reader, true);

				if (node)
					node->deserializeAttributes(attr);

				attr->drop();
			}
			else
			if (IRR_XML_FORMAT_MATERIALS == name)
				readMaterials(reader, node);
			else
			if (IRR_XML_FORMAT_ANIMATORS == name)
				readAnimators(reader, node);
			else
			if (IRR_XML_FORMAT_USERDATA  == name)
				readUserData(reader, node, userDataSerializer);
			else
			if ((IRR_XML_FORMAT_NODE  == name) ||
				(IRR_XML_FORMAT_SCENE == name))
			{
				readSceneNode(reader, node, userDataSerializer);
			}
			else
			{
				os::Printer::log("Found unknown element in irrlicht scene file",
						core::stringc(name).c_str());
			}
			break;
		default:
			break;
		}

		if (endreached)
			break;
	}
	if (node && userDataSerializer)
		userDataSerializer->OnCreateNode(node);
}

//! reads materials of a node
void CSceneLoaderIrr::readMaterials(io::IXMLReader* reader, ISceneNode* node)
{
	u32 nr = 0;

	while(reader->read())
	{
		const wchar_t* name = reader->getNodeName();

		switch(reader->getNodeType())
		{
		case io::EXN_ELEMENT_END:
			if (IRR_XML_FORMAT_MATERIALS == name)
				return;
			break;
		case io::EXN_ELEMENT:
			if (IRR_XML_FORMAT_ATTRIBUTES == name)
			{
				// read materials from attribute list
				io::IAttributes* attr = FileSystem->createEmptyAttributes(SceneManager->getVideoDriver());
				attr->read(reader);

				if (node && node->getMaterialCount() > nr)
				{
					SceneManager->getVideoDriver()->fillMaterialStructureFromAttributes(
						node->getMaterial(nr), attr);
				}

				attr->drop();
				++nr;
			}
			break;
		default:
			break;
		}
	}
}


//! reads animators of a node
void CSceneLoaderIrr::readAnimators(io::IXMLReader* reader, ISceneNode* node)
{
	while(reader->read())
	{
		const wchar_t* name = reader->getNodeName();

		switch(reader->getNodeType())
		{
		case io::EXN_ELEMENT_END:
			if (IRR_XML_FORMAT_ANIMATORS == name)
				return;
			break;
		case io::EXN_ELEMENT:
			if (IRR_XML_FORMAT_ATTRIBUTES == name)
			{
				// read animator data from attribute list
				io::IAttributes* attr = FileSystem->createEmptyAttributes(SceneManager->getVideoDriver());
				attr->read(reader);

				if (node)
				{
					core::stringc typeName = attr->getAttributeAsString("Type");
					ISceneNodeAnimator* anim = SceneManager->createSceneNodeAnimator(typeName.c_str(), node);

					if (anim)
					{
						anim->deserializeAttributes(attr);
						anim->drop();
					}
				}

				attr->drop();
			}
			break;
		default:
			break;
		}
	}
}


//! reads user data of a node
void CSceneLoaderIrr::readUserData(io::IXMLReader* reader, ISceneNode* node, ISceneUserDataSerializer* userDataSerializer)
{
	while(reader->read())
	{
		const wchar_t* name = reader->getNodeName();

		switch(reader->getNodeType())
		{
		case io::EXN_ELEMENT_END:
			if (IRR_XML_FORMAT_USERDATA == name)
				return;
			break;
		case io::EXN_ELEMENT:
			if (IRR_XML_FORMAT_ATTRIBUTES == name)
			{
				// read user data from attribute list
				io::IAttributes* attr = FileSystem->createEmptyAttributes(SceneManager->getVideoDriver());
				attr->read(reader);

				if (node && userDataSerializer)
				{
					userDataSerializer->OnReadUserData(node, attr);
				}

				attr->drop();
			}
			break;
		default:
			break;
		}
	}
}

} // scene
} // irr

