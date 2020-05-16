// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __C_DEFAULT_SCENE_NODE_FACTORY_H_INCLUDED__
#define __C_DEFAULT_SCENE_NODE_FACTORY_H_INCLUDED__

#include "ISceneNodeFactory.h"
#include "irrArray.h"
#include "irrString.h"

namespace irr
{
namespace scene
{
	class ISceneNode;
	class ISceneManager;
	
	//!	Interface making it possible to dynamicly create scene nodes and animators
	class CDefaultSceneNodeFactory : public ISceneNodeFactory
	{
	public:

		CDefaultSceneNodeFactory(ISceneManager* mgr);

		//! adds a scene node to the scene graph based on its type id
		/** \param type: Type of the scene node to add.
		\param parent: Parent scene node of the new node, can be null to add the scene node to the root.
		\return Returns pointer to the new scene node or null if not successful. */
		virtual ISceneNode* addSceneNode(ESCENE_NODE_TYPE type, ISceneNode* parent=0);

		//! adds a scene node to the scene graph based on its type name
		/** \param typeName: Type name of the scene node to add.
		\param parent: Parent scene node of the new node, can be null to add the scene node to the root.
		\return Returns pointer to the new scene node or null if not successful. */
		virtual ISceneNode* addSceneNode(const c8* typeName, ISceneNode* parent=0);

		//! returns amount of scene node types this factory is able to create
		virtual u32 getCreatableSceneNodeTypeCount() const;

		//! returns type name of a createable scene node type by index
		/** \param idx: Index of scene node type in this factory. Must be a value between 0 and
		uetCreatableSceneNodeTypeCount() */
		virtual const c8* getCreateableSceneNodeTypeName(u32 idx) const;

		//! returns type of a createable scene node type
		/** \param idx: Index of scene node type in this factory. Must be a value between 0 and
		getCreatableSceneNodeTypeCount() */
		virtual ESCENE_NODE_TYPE getCreateableSceneNodeType(u32 idx) const;

		//! returns type name of a createable scene node type 
		/** \param idx: Type of scene node. 
		\return: Returns name of scene node type if this factory can create the type, otherwise 0. */
		virtual const c8* getCreateableSceneNodeTypeName(ESCENE_NODE_TYPE type) const;

	private:

		ESCENE_NODE_TYPE getTypeFromName(const c8* name) const;

		struct SSceneNodeTypePair
		{
			SSceneNodeTypePair(ESCENE_NODE_TYPE type, const c8* name)
				: Type(type), TypeName(name)
			{}

			ESCENE_NODE_TYPE Type;
			core::stringc TypeName;
		};

		core::array<SSceneNodeTypePair> SupportedSceneNodeTypes;

		ISceneManager* Manager;
	};


} // end namespace scene
} // end namespace irr

#endif

