#include "OpenGL/VertexType.h"
#include "EVertexAttributes.h"
#include <cassert>

namespace irr
{
namespace video
{

const VertexAttribute *begin(const VertexType &type)
{
	return type.Attributes.data();
}

const VertexAttribute *end(const VertexType &type)
{
	return type.Attributes.data() + type.Attributes.size();
}

VertexType vtStandard = {
	sizeof(S3DVertex),
	{
		{video::EVA_POSITION, 3, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertex, Pos)},
		{video::EVA_NORMAL, 3, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertex, Normal)},
		{video::EVA_COLOR, 4, VertexAttribute::Type::UByte, VertexAttribute::Mode::Normalized, offsetof(S3DVertex, Color)},
		{video::EVA_TCOORD0, 2, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertex, TCoords)}
	}
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"

VertexType vt2TCoords = {
	sizeof(S3DVertex2TCoords),
	{
		{video::EVA_POSITION, 3, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertex2TCoords, Pos)},
		{video::EVA_NORMAL, 3, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertex2TCoords, Normal)},
		{video::EVA_COLOR, 4, VertexAttribute::Type::UByte, VertexAttribute::Mode::Normalized, offsetof(S3DVertex2TCoords, Color)},
		{video::EVA_TCOORD0, 2, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertex2TCoords, TCoords)},
		{video::EVA_TCOORD1, 2, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertex2TCoords, TCoords2)},
	},
};

VertexType vtTangents = {
	sizeof(S3DVertexTangents),
	{
		{video::EVA_POSITION, 3, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertexTangents, Pos)},
		{video::EVA_NORMAL, 3, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertexTangents, Normal)},
		{video::EVA_COLOR, 4, VertexAttribute::Type::UByte, VertexAttribute::Mode::Normalized, offsetof(S3DVertexTangents, Color)},
		{video::EVA_TCOORD0, 2, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertexTangents, TCoords)},
		{video::EVA_TANGENT, 3, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertexTangents, Tangent)},
		{video::EVA_BINORMAL, 3, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertexTangents, Binormal)},
	},
};

#pragma GCC diagnostic pop

const VertexType &getVertexTypeDescription(E_VERTEX_TYPE type)
{
	switch (type) {
	case EVT_STANDARD:
		return vtStandard;
	case EVT_2TCOORDS:
		return vt2TCoords;
	case EVT_TANGENTS:
		return vtTangents;
	default:
		assert(false);
	}
}

VertexType vt2DImage = {
		sizeof(S3DVertex),
		{
			{video::EVA_POSITION, 3, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertex, Pos)},
			{video::EVA_COLOR, 4, VertexAttribute::Type::UByte, VertexAttribute::Mode::Normalized, offsetof(S3DVertex, Color)},
			{video::EVA_TCOORD0, 2, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertex, TCoords)},
		},
};

VertexType vtPrimitive = {
		sizeof(S3DVertex),
		{
			{video::EVA_POSITION, 3, VertexAttribute::Type::Float, VertexAttribute::Mode::Regular, offsetof(S3DVertex, Pos)},
			{video::EVA_COLOR, 4, VertexAttribute::Type::UByte, VertexAttribute::Mode::Normalized, offsetof(S3DVertex, Color)},
		},
};

}
}
