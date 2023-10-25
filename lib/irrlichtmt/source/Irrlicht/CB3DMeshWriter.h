// Copyright (C) 2014 Lauri Kasanen
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

// Modified version with rigging/skinning support

#pragma once

#include "IMeshWriter.h"
#include "IWriteFile.h"
#include "SB3DStructs.h"
#include "ISkinnedMesh.h"



namespace irr
{
namespace scene
{

//! class to write B3D mesh files
class CB3DMeshWriter : public IMeshWriter
{
public:

	CB3DMeshWriter();

	//! Returns the type of the mesh writer
    EMESH_WRITER_TYPE getType() const override;

	//! writes a mesh
    bool writeMesh(io::IWriteFile* file, scene::IMesh* mesh, s32 flags=EMWF_NONE) override;

private:
    void writeJointChunk(io::IWriteFile* file, ISkinnedMesh* mesh , ISkinnedMesh::SJoint* joint, f32 animationSpeedMultiplier);
    u32 getJointChunkSize(const ISkinnedMesh* mesh, ISkinnedMesh::SJoint* joint);
    core::array<ISkinnedMesh::SJoint*> getRootJoints(const ISkinnedMesh* mesh);

    u32 getUVlayerCount(const IMesh *mesh);
    ISkinnedMesh* getSkinned (IMesh *mesh);

    inline void writeVector2(io::IWriteFile* file, const core::vector2df& vec);
    inline void writeVector3(io::IWriteFile* file, const core::vector3df& vec);
    inline void writeQuaternion(io::IWriteFile* file, const core::quaternion& quat);
    inline void writeColor(io::IWriteFile* file, const video::SColorf& color);
    void writeSizeFrom(io::IWriteFile* file, const u32 from, const u32 adressToWrite);
};

} // end namespace
} // end namespace

