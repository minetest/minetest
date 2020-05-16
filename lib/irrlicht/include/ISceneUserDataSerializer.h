// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __I_SCENE_USER_DATA_SERIALIZER_H_INCLUDED__
#define __I_SCENE_USER_DATA_SERIALIZER_H_INCLUDED__

#include "IReferenceCounted.h"

namespace irr
{
namespace io
{
	class IAttributes;
} // end namespace io
namespace scene
{
	class ISceneNode;

//! Interface to read and write user data to and from .irr files.
/** This interface is to be implemented by the user, to make it possible to read
and write user data when reading or writing .irr files via ISceneManager.
To be used with ISceneManager::loadScene() and ISceneManager::saveScene() */
class ISceneUserDataSerializer
{
public:

	virtual ~ISceneUserDataSerializer() {}

	//! Called when the scene manager create a scene node while loading a file.
	virtual void OnCreateNode(ISceneNode* node) = 0;
	
	//! Called when the scene manager read a scene node while loading a file.
	/** The userData pointer contains a list of attributes with userData which
	were attached to the scene node in the read scene file.*/
	virtual void OnReadUserData(ISceneNode* forSceneNode, io::IAttributes* userData) = 0;

	//! Called when the scene manager is writing a scene node to an xml file for example.
	/** Implement this method and return a list of attributes containing the user data
	you want to be saved together with the scene node. Return 0 if no user data should
	be added. Please note that the scene manager will call drop() to the returned pointer
	after it no longer needs it, so if you didn't create a new object for the return value
	and returning a longer existing IAttributes object, simply call grab() before returning it. */
	virtual io::IAttributes* createUserData(ISceneNode* forSceneNode) = 0;
};

} // end namespace scene
} // end namespace irr

#endif

