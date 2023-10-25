// Copyright (C) 2014 Lauri Kasanen
// This file is part of the "Irrlicht Engine".
// For conditions of distribution and use, see copyright notice in irrlicht.h

// TODO: replace printf's by logging messages


#include "CB3DMeshWriter.h"
#include "os.h"
#include "ISkinnedMesh.h"
#include "IMeshBuffer.h"
#include "IWriteFile.h"
#include "ITexture.h"


namespace irr
{
namespace scene
{

using namespace core;
using namespace video;

CB3DMeshWriter::CB3DMeshWriter()
{
	#ifdef _DEBUG
	setDebugName("CB3DMeshWriter");
	#endif
}


//! Returns the type of the mesh writer
EMESH_WRITER_TYPE CB3DMeshWriter::getType() const
{
    return EMWT_B3D;
}


//! writes a mesh
bool CB3DMeshWriter::writeMesh(io::IWriteFile* file, IMesh* const mesh, s32 flags)
{
    if (!file || !mesh)
        return false;
#ifdef __BIG_ENDIAN__
    os::Printer::log("B3D export does not support big-endian systems.", ELL_ERROR);
    return false;
#endif

    file->write("BB3D", 4);
    file->write("size", 4); // BB3D chunk size, updated later

    const u32 version = 1;
    file->write(&version, 4);

    //

    const u32 numMeshBuffers = mesh->getMeshBufferCount();
    array<SB3dTexture> texs;
    std::map<ITexture *, u32> tex2id;	// TODO: texture pointer as key not sufficient as same texture can have several id's
    u32 texsizes = 0;
    for (u32 i = 0; i < numMeshBuffers; i++)
	{
        const IMeshBuffer * const mb = mesh->getMeshBuffer(i);
        const SMaterial &mat = mb->getMaterial();

        for (u32 j = 0; j < MATERIAL_MAX_TEXTURES; j++)
		{
            if (mat.getTexture(j))
			{
                SB3dTexture t;
				t.TextureName = core::stringc(mat.getTexture(j)->getName().getPath());

				// TODO: need some description of Blitz3D texture-flags to figure this out. But Blend should likely depend on material-type.
				t.Flags = j == 2 ? 65536 : 1;
				t.Blend = 2;

				// TODO: evaluate texture matrix
				t.Xpos = 0;
				t.Ypos = 0;
				t.Xscale = 1;
				t.Yscale = 1;
				t.Angle = 0;

                texs.push_back(t);
                texsizes += 7*4 + t.TextureName.size() + 1;
                tex2id[mat.getTexture(j)] = texs.size() - 1;
            }
        }
    }

    file->write("TEXS", 4);
    file->write(&texsizes, 4);

    u32 numTexture = texs.size();
    for (u32 i = 0; i < numTexture; i++)
	{
        file->write(texs[i].TextureName.c_str(), (size_t)texs[i].TextureName.size() + 1);
        file->write(&texs[i].Flags, 7*4);
    }

    //

    file->write("BRUS", 4);
    const u32 brushSizeAdress = file->getPos();
    file->write(&brushSizeAdress, 4); // BRUSH chunk size, updated later

    const u32 usedtex = MATERIAL_MAX_TEXTURES;
    file->write(&usedtex, 4);

    for (u32 i = 0; i < numMeshBuffers; i++)
	{
        const IMeshBuffer * const mb = mesh->getMeshBuffer(i);
        const SMaterial &mat = mb->getMaterial();

        file->write("", 1);

        float f = 1;
        file->write(&f, 4);
        file->write(&f, 4);
        file->write(&f, 4);
        file->write(&f, 4);

        f = 0;
        file->write(&f, 4);

        u32 tmp = 1;
        file->write(&tmp, 4);
        tmp = 0;
        file->write(&tmp, 4);

        for (u32 j = 0; j < MATERIAL_MAX_TEXTURES; j++)
		{
		    s32 id = -1;
            if (mat.getTexture(j))
			{
                id = tex2id[mat.getTexture(j)];
            }
            file->write(&id, 4);
        }
    }
    writeSizeFrom(file, brushSizeAdress+4, brushSizeAdress); // BRUSH chunk size

    file->write("NODE", 4);
    u32 nodeSizeAdress = file->getPos();
    file->write(&nodeSizeAdress, 4); // NODE chunk size, updated later

    // Node
    file->write("", 1);

    // position
    writeVector3(file, core::vector3df(0.f, 0.f, 0.f));

    // scale
    writeVector3(file, core::vector3df(1.f, 1.f, 1.f));

    // rotation
    writeQuaternion(file, core::quaternion(0.f, 0.f, 0.f, 1.f));

    // Mesh
    file->write("MESH", 4);
    const u32 meshSizeAdress = file->getPos();
    file->write(&meshSizeAdress, 4); // MESH chunk size, updated later

	s32 brushID = -1;
    file->write(&brushID, 4);



    // Verts
    file->write("VRTS", 4);
    const u32 verticesSizeAdress = file->getPos();
    file->write(&verticesSizeAdress, 4);

    u32 flagsB3D = 3; // 1=normal values present, 2=rgba values present
    file->write(&flagsB3D, 4);

    const u32 texcoordsCount = getUVlayerCount(mesh);
    file->write(&texcoordsCount, 4);
    flagsB3D = 2;
    file->write(&flagsB3D, 4);

    for (u32 i = 0; i < numMeshBuffers; i++)
    {
        const IMeshBuffer * const mb = mesh->getMeshBuffer(i);
        const u32 numVertices = mb->getVertexCount();
        for (u32 j = 0; j < numVertices; j++)
		{
            const vector3df &pos = mb->getPosition(j);
            writeVector3(file, pos);

            const vector3df &n = mb->getNormal(j);
            writeVector3(file, n);

            switch (mb->getVertexType())
			{
                case EVT_STANDARD:
                {
                    S3DVertex *v = (S3DVertex *) mb->getVertices();
                    const SColorf col(v[j].Color);
                    writeColor(file, col);

                    const core::vector2df uv1 = v[j].TCoords;
                    writeVector2(file, uv1);
                    if (texcoordsCount == 2)
                    {
                        writeVector2(file, core::vector2df(0.f, 0.f));
                    }
                }
                break;
                case EVT_2TCOORDS:
                {
                    S3DVertex2TCoords *v = (S3DVertex2TCoords *) mb->getVertices();
                    const SColorf col(v[j].Color);
                    writeColor(file, col);

                    const core::vector2df uv1 = v[j].TCoords;
                    writeVector2(file, uv1);
                    const core::vector2df uv2 = v[j].TCoords;
                    writeVector2(file, uv2);
                }
                break;
                case EVT_TANGENTS:
                {
                    S3DVertexTangents *v = (S3DVertexTangents *) mb->getVertices();
                    const SColorf col(v[j].Color);
                    writeColor(file, col);

                    const core::vector2df uv1 = v[j].TCoords;
                    writeVector2(file, uv1);
                    if (texcoordsCount == 2)
                    {
                        writeVector2(file, core::vector2df(0.f, 0.f));
                    }
                }
                break;
            }
        }
    }
    writeSizeFrom(file, verticesSizeAdress+4, verticesSizeAdress); // VERT chunk size


    u32 currentMeshBufferIndex = 0;
    // Tris
    for (u32 i = 0; i < numMeshBuffers; i++)
    {
        const IMeshBuffer * const mb = mesh->getMeshBuffer(i);
        file->write("TRIS", 4);
        const u32 trisSizeAdress = file->getPos();
        file->write(&trisSizeAdress, 4); // TRIS chunk size, updated later

        file->write(&i, 4);

        u32 numIndices = mb->getIndexCount();
        const u16 * const idx = (u16 *) mb->getIndices();
        for (u32 j = 0; j < numIndices; j += 3)
		{
            u32 tmp = idx[j] + currentMeshBufferIndex;
            file->write(&tmp, sizeof(u32));

            tmp = idx[j + 1] + currentMeshBufferIndex;
            file->write(&tmp, sizeof(u32));

            tmp = idx[j + 2] + currentMeshBufferIndex;
            file->write(&tmp, sizeof(u32));
        }
        writeSizeFrom(file, trisSizeAdress+4, trisSizeAdress);  // TRIS chunk size

        currentMeshBufferIndex += mb->getVertexCount();
    }
    writeSizeFrom(file, meshSizeAdress+4, meshSizeAdress); // MESH chunk size


    if(ISkinnedMesh *skinnedMesh = getSkinned(mesh))
    {
        // Write animation data
        f32 animationSpeedMultiplier = 1.f;
        if (!skinnedMesh->isStatic())
        {
            file->write("ANIM", 4);

            const u32 animsize = 12;
            file->write(&animsize, 4);

            const u32 flags = 0;
            f32 fps = skinnedMesh->getAnimationSpeed();

            /* B3D file format use integer as keyframe, so there is some potential issues if the model use float as keyframe (Irrlicht use float) with a low animation FPS value
            So we define a minimum animation FPS value to multiply the frame and FPS value if the FPS of the animation is too low to store the keyframe with integers */
            const int minimumAnimationFPS = 60;

            if (fps < minimumAnimationFPS)
            {
                animationSpeedMultiplier = minimumAnimationFPS / fps;
                fps = minimumAnimationFPS;
            }
            const u32 frames = static_cast<u32>(skinnedMesh->getFrameCount() * animationSpeedMultiplier);

            file->write(&flags, 4);
            file->write(&frames, 4);
            file->write(&fps, 4);
        }

        // Write joints
        core::array<ISkinnedMesh::SJoint*> rootJoints = getRootJoints(skinnedMesh);

        for (u32 i = 0; i < rootJoints.size(); i++)
        {
            writeJointChunk(file, skinnedMesh, rootJoints[i], animationSpeedMultiplier);
        }
    }

    writeSizeFrom(file, nodeSizeAdress+4, nodeSizeAdress); // Node chunk size
	writeSizeFrom(file, 8, 4); // BB3D chunk size

    return true;
}



void CB3DMeshWriter::writeJointChunk(io::IWriteFile* file, ISkinnedMesh* mesh, ISkinnedMesh::SJoint* joint, f32 animationSpeedMultiplier)
{
    // Node
    file->write("NODE", 4);
    const u32 nodeSizeAdress = file->getPos();
    file->write(&nodeSizeAdress, 4);


    core::stringc name = joint->Name;
    file->write(name.c_str(), name.size());
    file->write("", 1);

    // Position
    const core::vector3df pos = joint->Animatedposition;
    writeVector3(file, pos);

    // Scale
    core::vector3df scale = joint->Animatedscale;
    if (scale == core::vector3df(0, 0, 0))
        scale = core::vector3df(1, 1, 1);

    writeVector3(file, scale);

    // Rotation
    const core::quaternion quat = joint->Animatedrotation;
    writeQuaternion(file, quat);

    // Bone
    file->write("BONE", 4);
    u32 bonesize = 8 * joint->Weights.size();
    file->write(&bonesize, 4);

    // Skinning ------------------
    for (u32 i = 0; i < joint->Weights.size(); i++)
    {
        const u32 vertexID = joint->Weights[i].vertex_id;
        const u32 bufferID = joint->Weights[i].buffer_id;
        const f32 weight = joint->Weights[i].strength;

        u32 b3dVertexID = vertexID;
        for (u32 j = 0; j < bufferID; j++)
        {
            b3dVertexID += mesh->getMeshBuffer(j)->getVertexCount();
        }

        file->write(&b3dVertexID, 4);
        file->write(&weight, 4);
    }
    // ---------------------------

    f32 floatBuffer[5];
    // Animation keys
    if (joint->PositionKeys.size())
    {
        file->write("KEYS", 4);
        u32 keysSize = 4 * joint->PositionKeys.size() * 4; // X, Y and Z pos + frame
        keysSize += 4;  // Flag to define the type of the key
        file->write(&keysSize, 4);

        u32 flag = 1; // 1 = flag for position keys
        file->write(&flag, 4);

        for (u32 i = 0; i < joint->PositionKeys.size(); i++)
        {
            const s32 frame = static_cast<s32>(joint->PositionKeys[i].frame * animationSpeedMultiplier);
            file->write(&frame, 4);

            const core::vector3df pos = joint->PositionKeys[i].position;
            pos.getAs3Values(floatBuffer);
            file->write(floatBuffer, 12);
        }
    }
    if (joint->RotationKeys.size())
    {
        file->write("KEYS", 4);
        u32 keysSize = 4 * joint->RotationKeys.size() * 5; // W, X, Y and Z rot + frame
        keysSize += 4; // Flag
        file->write(&keysSize, 4);

        u32 flag = 4;
        file->write(&flag, 4);

        for (u32 i = 0; i < joint->RotationKeys.size(); i++)
        {
            const s32 frame = static_cast<s32>(joint->RotationKeys[i].frame * animationSpeedMultiplier);
            const core::quaternion rot = joint->RotationKeys[i].rotation;

            memcpy(floatBuffer, &frame, 4);
            floatBuffer[1] = rot.W;
            floatBuffer[2] = rot.X;
            floatBuffer[3] = rot.Y;
            floatBuffer[4] = rot.Z;
            file->write(floatBuffer, 20);
        }
    }
    if (joint->ScaleKeys.size())
    {
        file->write("KEYS", 4);
        u32 keysSize = 4 * joint->ScaleKeys.size() * 4; // X, Y and Z scale + frame
        keysSize += 4; // Flag
        file->write(&keysSize, 4);

        u32 flag = 2;
        file->write(&flag, 4);

        for (u32 i = 0; i < joint->ScaleKeys.size(); i++)
        {
            const s32 frame = static_cast<s32>(joint->ScaleKeys[i].frame * animationSpeedMultiplier);
            file->write(&frame, 4);

            const core::vector3df scale = joint->ScaleKeys[i].scale;
            scale.getAs3Values(floatBuffer);
            file->write(floatBuffer, 12);
        }
    }

    for (u32 i = 0; i < joint->Children.size(); i++)
    {
        writeJointChunk(file, mesh, joint->Children[i], animationSpeedMultiplier);
    }

    writeSizeFrom(file, nodeSizeAdress+4, nodeSizeAdress); // NODE chunk size
}


ISkinnedMesh* CB3DMeshWriter::getSkinned (IMesh *mesh)
{
	if (mesh->getMeshType() == EAMT_SKINNED)
    {
		return static_cast<ISkinnedMesh*>(mesh);
    }
    return 0;
}

core::array<ISkinnedMesh::SJoint*> CB3DMeshWriter::getRootJoints(const ISkinnedMesh* mesh)
{
    core::array<ISkinnedMesh::SJoint*> roots;

    core::array<ISkinnedMesh::SJoint*> allJoints = mesh->getAllJoints();
    for (u32 i = 0; i < allJoints.size(); i++)
    {
        bool isRoot = true;
        ISkinnedMesh::SJoint* testedJoint = allJoints[i];
        for (u32 j = 0; j < allJoints.size(); j++)
        {
           ISkinnedMesh::SJoint* testedJoint2 = allJoints[j];
           for (u32 k = 0; k < testedJoint2->Children.size(); k++)
           {
               if (testedJoint == testedJoint2->Children[k])
                    isRoot = false;
           }
        }
        if (isRoot)
            roots.push_back(testedJoint);
    }

    return roots;
}

u32 CB3DMeshWriter::getUVlayerCount(const IMesh* mesh)
{
    const u32 numBeshBuffers = mesh->getMeshBufferCount();
    for (u32 i = 0; i < numBeshBuffers; i++)
    {
        const IMeshBuffer * const mb = mesh->getMeshBuffer(i);

        if (mb->getVertexType() == EVT_2TCOORDS)
        {
            return 2;
        }
    }
    return 1;
}

void CB3DMeshWriter::writeVector2(io::IWriteFile* file, const core::vector2df& vec2)
{
    f32 buffer[2] = {vec2.X, vec2.Y};
    file->write(buffer, 8);
}

void CB3DMeshWriter::writeVector3(io::IWriteFile* file, const core::vector3df& vec3)
{
    f32 buffer[3];
    vec3.getAs3Values(buffer);
    file->write(buffer, 12);
}

void CB3DMeshWriter::writeQuaternion(io::IWriteFile* file, const core::quaternion& quat)
{
    f32 buffer[4] = {quat.W, quat.X, quat.Y, quat.Z};
    file->write(buffer, 16);
}

void CB3DMeshWriter::writeColor(io::IWriteFile* file, const video::SColorf& color)
{
    f32 buffer[4] = {color.r, color.g, color.b, color.a};
    file->write(buffer, 16);
}

// Write the size from a given position to current position at a specific position in the file
void CB3DMeshWriter::writeSizeFrom(io::IWriteFile* file, const u32 from, const u32 adressToWrite)
{
    const long back = file->getPos();
    file->seek(adressToWrite);
    const u32 sizeToWrite = back - from;
    file->write(&sizeToWrite, 4);
    file->seek(back);
}

} // end namespace
} // end namespace
