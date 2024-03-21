// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#pragma once

#include "irrTypes.h"

/*! \file SceneParameters.h
	\brief Header file containing all scene parameters for modifying mesh loading etc.

	This file includes all parameter names which can be set using ISceneManager::getParameters()
	to modify the behavior of plugins and mesh loaders.
*/

namespace irr
{
namespace scene
{
//! Name of the parameter for changing how Irrlicht handles the ZWrite flag for transparent (blending) materials
/** The default behavior in Irrlicht is to disable writing to the
z-buffer for all really transparent, i.e. blending materials. This
avoids problems with intersecting faces, but can also break renderings.
If transparent materials should use the SMaterial flag for ZWriteEnable
just as other material types use this attribute.
Use it like this:
\code
SceneManager->getParameters()->setAttribute(scene::ALLOW_ZWRITE_ON_TRANSPARENT, true);
\endcode
**/
const c8 *const ALLOW_ZWRITE_ON_TRANSPARENT = "Allow_ZWrite_On_Transparent";

//! Flag to avoid loading group structures in .obj files
/** Use it like this:
\code
SceneManager->getParameters()->setAttribute(scene::OBJ_LOADER_IGNORE_GROUPS, true);
\endcode
**/
const c8 *const OBJ_LOADER_IGNORE_GROUPS = "OBJ_IgnoreGroups";

//! Flag to avoid loading material .mtl file for .obj files
/** Use it like this:
\code
SceneManager->getParameters()->setAttribute(scene::OBJ_LOADER_IGNORE_MATERIAL_FILES, true);
\endcode
**/
const c8 *const OBJ_LOADER_IGNORE_MATERIAL_FILES = "OBJ_IgnoreMaterialFiles";

} // end namespace scene
} // end namespace irr
