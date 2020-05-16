// Copyright (C) 2002-2012 Nikolaus Gebhardt
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

#ifndef __IRR_I_COLLADA_MESH_WRITER_H_INCLUDED__
#define __IRR_I_COLLADA_MESH_WRITER_H_INCLUDED__

#include "IMeshWriter.h"
#include "ISceneNode.h"
#include "IAnimatedMesh.h"
#include "SMaterial.h"

namespace irr
{
namespace io
{
	class IWriteFile;
} // end namespace io

namespace scene
{
	//! Lighting models - more or less the way Collada categorizes materials
	enum E_COLLADA_TECHNIQUE_FX
	{
		//! Blinn-phong which is default for opengl and dx fixed function pipelines.
		//! But several well-known renderers don't support it and prefer phong.
		ECTF_BLINN,	
		//! Phong shading, default in many external renderers.
		ECTF_PHONG,
		//! diffuse shaded surface that is independent of lighting.
		ECTF_LAMBERT,
		// constantly shaded surface that is independent of lighting. 
		ECTF_CONSTANT
	};

	//! How to interpret the opacity in collada
	enum E_COLLADA_TRANSPARENT_FX
	{
		//! default - only alpha channel of color or texture is used.
		ECOF_A_ONE = 0,

		//! Alpha values for each RGB channel of color or texture are used. 
		ECOF_RGB_ZERO = 1
	};

	//! Color names collada uses in it's color samplers
	enum E_COLLADA_COLOR_SAMPLER
	{
		ECCS_DIFFUSE,
		ECCS_AMBIENT,
		ECCS_EMISSIVE,
		ECCS_SPECULAR,
		ECCS_TRANSPARENT,
		ECCS_REFLECTIVE
	};

	//! Irrlicht colors which can be mapped to E_COLLADA_COLOR_SAMPLER values
	enum E_COLLADA_IRR_COLOR
	{
		//! Don't write this element at all
		ECIC_NONE,

		//! Check IColladaMeshWriterProperties for custom color
		ECIC_CUSTOM,

		//! Use SMaterial::DiffuseColor
		ECIC_DIFFUSE,

		//! Use SMaterial::AmbientColor
		ECIC_AMBIENT,

		//! Use SMaterial::EmissiveColor
		ECIC_EMISSIVE,

		//! Use SMaterial::SpecularColor
		ECIC_SPECULAR
	};

	//! Control when geometry elements are created
	enum E_COLLADA_GEOMETRY_WRITING
	{
		//! Default - write each mesh exactly once to collada. Optimal but will not work with many tools.
		ECGI_PER_MESH,	

		//! Write each mesh as often as it's used with different materials-names in the scene.
		//! Material names which are used here are created on export, so using the IColladaMeshWriterNames
		//! interface you have some control over how many geometries are written.
		ECGI_PER_MESH_AND_MATERIAL
	};

	//! Callback interface for properties which can be used to influence collada writing
	class IColladaMeshWriterProperties  : public virtual IReferenceCounted
	{
	public:
		virtual ~IColladaMeshWriterProperties ()	{}

		//! Which lighting model should be used in the technique (FX) section when exporting effects (materials)
		virtual E_COLLADA_TECHNIQUE_FX getTechniqueFx(const video::SMaterial& material) const = 0;

		//! Which texture index should be used when writing the texture of the given sampler color.
		/** \return the index to the texture-layer or -1 if that texture should never be exported 
			Note: for ECCS_TRANSPARENT by default the alpha channel is used, if you want to use RGB you have to set
			also the ECOF_RGB_ZERO flag in getTransparentFx.  */
		virtual s32 getTextureIdx(const video::SMaterial & material, E_COLLADA_COLOR_SAMPLER cs) const = 0;

		//! Return which color from Irrlicht should be used for the color requested by collada
		/** Note that collada allows exporting either texture or color, not both. 
			So color mapping is only checked if we have no valid texture already.
			By default we try to return best fits when possible. For example ECCS_DIFFUSE is mapped to ECIC_DIFFUSE.
			When ECIC_CUSTOM is returned then the result of getCustomColor will be used. */
		virtual E_COLLADA_IRR_COLOR getColorMapping(const video::SMaterial & material, E_COLLADA_COLOR_SAMPLER cs) const = 0;

		//! Return custom colors for certain color types requested by collada. 
		/** Only used when getColorMapping returns ECIC_CUSTOM for the same paramters. */
		virtual video::SColor getCustomColor(const video::SMaterial & material, E_COLLADA_COLOR_SAMPLER cs) const = 0;

		//! Return the transparence color interpretation.
		/** Not this is only about ECCS_TRANSPARENT and does not affect getTransparency. */
		virtual E_COLLADA_TRANSPARENT_FX getTransparentFx(const video::SMaterial& material) const = 0;

		//! Transparency value for that material. 
		/** This value is additional to transparent settings, if both are set they will be multiplicated.
		\return 1.0 for fully transparent, 0.0 for not transparent and not written at all when < 0.f */
		virtual f32 getTransparency(const video::SMaterial& material) const = 0;

		//! Reflectivity value for that material
		/** The amount of perfect mirror reflection to be added to the reflected light 
		\return 0.0 - 1.0 for reflectivity and element is not written at all when < 0.f */
		virtual f32 getReflectivity(const video::SMaterial& material) const = 0;

		//! Return index of refraction for that material
		/**	By default we don't write that.
		\return a value greater equal 0.f to write \<index_of_refraction\> when it is lesser than 0 nothing will be written */
		virtual f32 getIndexOfRefraction(const video::SMaterial& material) const = 0;

		//! Should node be used in scene export? (only needed for scene-writing, ignored in mesh-writing)
		//! By default all visible nodes are exported.
		virtual bool isExportable(const irr::scene::ISceneNode * node) const = 0;

		//! Return the mesh for the given node. If it has no mesh or shouldn't export it's mesh 
		//! you can return 0 in which case only the transformation matrix of the node will be used.
		// Note: Function is not const because there is no const getMesh() function.
		virtual IMesh* getMesh(irr::scene::ISceneNode * node) = 0;

		//! Return if the node has it's own material overwriting the mesh-materials
		/** Usually true except for mesh-nodes which have isReadOnlyMaterials set.
		This is mostly important for naming (as ISceneNode::getMaterial() already returns the correct material).
		You have to override it when exporting custom scenenodes with own materials.
		\return true => The node's own material is used, false => ignore node material and use the one from the mesh */
		virtual bool useNodeMaterial(const scene::ISceneNode* node) const = 0;

	};

	//! Callback interface to use custom names on collada writing.
	/** You can either modify names and id's written to collada or you can use
	this interface to just find out which names are used on writing.
	*/
	class IColladaMeshWriterNames  : public virtual IReferenceCounted
	{
	public:
	
		virtual ~IColladaMeshWriterNames () {}

		//! Return a unique name for the given mesh
		/** Note that names really must be unique here per mesh-pointer, so 
		mostly it's a good idea to return the nameForMesh from 
		IColladaMeshWriter::getDefaultNameGenerator(). Also names must follow 
		the xs::NCName standard to be valid, you can run them through 
		IColladaMeshWriter::toNCName to ensure that.
		\param mesh Pointer to the mesh which needs a name
		\param instance When E_COLLADA_GEOMETRY_WRITING is not ECGI_PER_MESH then 
		several instances of the same mesh can be written and this counts them.
		*/
		virtual irr::core::stringw nameForMesh(const scene::IMesh* mesh, int instance) = 0;

		//! Return a unique name for the given node
		/** Note that names really must be unique here per node-pointer, so 
		mostly it's a good idea to return the nameForNode from 
		IColladaMeshWriter::getDefaultNameGenerator(). Also names must follow 
		the xs::NCName standard to be valid, you can run them through 
		IColladaMeshWriter::toNCName to ensure that.
		*/
		virtual irr::core::stringw nameForNode(const scene::ISceneNode* node) = 0;

		//! Return a name for the material
		/** There is one material created in the writer for each unique name. 
		So you can use this to control the number of materials which get written. 
		For example Irrlicht does by default write one material for each material
		instanced by a node. So if you know that in your application material 
		instances per node are identical between different nodes you can reduce 
		the number of exported materials using that knowledge by using identical 
		names for such shared materials. 
		Names must follow the xs::NCName standard to be valid, you can run them 
		through	IColladaMeshWriter::toNCName to ensure that.
		*/
		virtual irr::core::stringw nameForMaterial(const video::SMaterial & material, int materialId, const scene::IMesh* mesh, const scene::ISceneNode* node) = 0;
	};


	//! Interface for writing meshes
	class IColladaMeshWriter : public IMeshWriter
	{
	public:

		IColladaMeshWriter() 
			: Properties(0), DefaultProperties(0), NameGenerator(0), DefaultNameGenerator(0)
			, WriteTextures(true), WriteDefaultScene(true), ExportSMaterialOnce(true)
			, AmbientLight(0.f, 0.f, 0.f, 1.f)
			, GeometryWriting(ECGI_PER_MESH)
		{
		}

		//! Destructor
		virtual ~IColladaMeshWriter() 
		{
			if ( Properties )
				Properties->drop();
			if ( DefaultProperties )
				DefaultProperties->drop();
			if ( NameGenerator )
				NameGenerator->drop();
			if ( DefaultNameGenerator )
				DefaultNameGenerator->drop();
		}

		//! writes a scene starting with the given node
		virtual bool writeScene(io::IWriteFile* file, scene::ISceneNode* root) = 0;


		//! Set if texture information should be written
		virtual void setWriteTextures(bool write)
		{
			WriteTextures = write;
		}

		//! Get if texture information should be written
		virtual bool getWriteTextures() const 
		{
			return WriteTextures;
		}

		//! Set if a default scene should be written when writing meshes.
		/** Many collada readers fail to read a mesh if the collada files doesn't contain a scene as well.
		The scene is doing an instantiation of the mesh.
		When using writeScene this flag is ignored (as we have scene there already)
		*/
		virtual void setWriteDefaultScene(bool write)
		{
			WriteDefaultScene = write;
		}

		//! Get if a default scene should be written
		virtual bool getWriteDefaultScene() const
		{
			return WriteDefaultScene;
		}

		//! Sets ambient color of the scene to write
		virtual void setAmbientLight(const video::SColorf &ambientColor)
		{
			AmbientLight = ambientColor;
		}

		//! Return ambient light of the scene which is written
		virtual video::SColorf getAmbientLight() const
		{
			return AmbientLight;
		}

		//! Control when and how often a mesh is written
		/** Optimally ECGI_PER_MESH would be always sufficent - writing geometry once per mesh.
		Unfortunately many tools (at the time of writing this nearly all of them) have trouble
		on import when different materials are used per node. So when you override materials
		per node and importing the resuling collada has materials problems in other tools try 
		using other values here. 
		\param writeStyle One of the E_COLLADA_GEOMETRY_WRITING settings. 
		*/
		virtual void setGeometryWriting(E_COLLADA_GEOMETRY_WRITING writeStyle) 
		{
			GeometryWriting = writeStyle;
		}

		//! Get the current style of geometry writing.
		virtual E_COLLADA_GEOMETRY_WRITING getGeometryWriting() const
		{
			return GeometryWriting;
		}

		//! Make certain there is only one collada material generated per Irrlicht material
		/** Checks before creating a collada material-name if an identical 
		irr:::video::SMaterial has been exported already. If so don't export it with 
		another name. This is set by default and leads to way smaller .dae files.
		Note that if you need to disable this flag for some reason you can still 
		get a similar effect using the IColladaMeshWriterNames::nameForMaterial
		by returning identical names for identical materials there.
		*/
		virtual void setExportSMaterialsOnlyOnce(bool exportOnce)
		{
			ExportSMaterialOnce = exportOnce;
		}

		virtual bool getExportSMaterialsOnlyOnce() const
		{
			return ExportSMaterialOnce;
		}

		//! Set properties to use by the meshwriter instead of it's default properties.
		/** Overloading properties with an own class allows modifying the writing process in certain ways. 
		By default properties are set to the DefaultProperties. */
		virtual void setProperties(IColladaMeshWriterProperties * p)
		{
			if ( p == Properties )
				return;
			if ( p ) 
				p->grab();
			if ( Properties )
				Properties->drop();
			Properties = p;
		}

		//! Get properties which are currently used.
		virtual IColladaMeshWriterProperties * getProperties() const
		{
			return Properties;
		}

		//! Return the original default properties of the writer. 
		/** You can use this pointer in your own properties to access and return default values. */
		IColladaMeshWriterProperties * getDefaultProperties() const 
		{ 
			return DefaultProperties; 
		}

		//! Install a generator to create custom names on export. 
		virtual void setNameGenerator(IColladaMeshWriterNames * nameGenerator)
		{
			if ( nameGenerator == NameGenerator )
				return;
			if ( nameGenerator ) 
				nameGenerator->grab();
			if ( NameGenerator )
				NameGenerator->drop();
			NameGenerator = nameGenerator;
		}

		//! Get currently used name generator
		virtual IColladaMeshWriterNames * getNameGenerator() const
		{
			return NameGenerator;
		}

		//! Return the original default name generator of the writer. 
		/** You can use this pointer in your own generator to access and return default values. */
		IColladaMeshWriterNames * getDefaultNameGenerator() const 
		{ 
			return DefaultNameGenerator; 
		}

		//! Restrict the characters of oldString a set of allowed characters in xs::NCName and add the prefix.
		/** A tool function to help when using a custom name generator to generative valid names for collada names and id's. */
		virtual irr::core::stringw toNCName(const irr::core::stringw& oldString, const irr::core::stringw& prefix=irr::core::stringw(L"_NC_")) const = 0;


	protected:
		// NOTE: You usually should also call setProperties with the same paraemter when using setDefaultProperties
		virtual void setDefaultProperties(IColladaMeshWriterProperties * p)
		{
			if ( p == DefaultProperties )
				return;
			if ( p ) 
				p->grab();
			if ( DefaultProperties )
				DefaultProperties->drop();
			DefaultProperties = p;
		}

		// NOTE: You usually should also call setNameGenerator with the same paraemter when using setDefaultProperties
		virtual void setDefaultNameGenerator(IColladaMeshWriterNames * p)
		{
			if ( p == DefaultNameGenerator )
				return;
			if ( p ) 
				p->grab();
			if ( DefaultNameGenerator )
				DefaultNameGenerator->drop();
			DefaultNameGenerator = p;
		}

	private:
		IColladaMeshWriterProperties * Properties;
		IColladaMeshWriterProperties * DefaultProperties;
		IColladaMeshWriterNames * NameGenerator;
		IColladaMeshWriterNames * DefaultNameGenerator;
		bool WriteTextures;
		bool WriteDefaultScene;
		bool ExportSMaterialOnce;
		video::SColorf AmbientLight;
		E_COLLADA_GEOMETRY_WRITING GeometryWriting;
	};


} // end namespace
} // end namespace

#endif
