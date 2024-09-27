#pragma once

#include <vector>
#include <cstddef>
#include "S3DVertex.h"

namespace irr
{
namespace video
{
struct VertexAttribute
{
	enum class Mode
	{
		Regular,
		Normalized,
		Integral,
	};

	enum class Type
	{
		Byte = 0x1400,
		UByte,
		Short,
		UShort,
		Int,
		UInt,
		Float,
		TwoBytes,
		ThreeBytes,
		FourBytes,
		Double
	};

	int Index;
	int ComponentCount;
	Type ComponentType;
	Mode mode;
	size_t Offset;
};

struct VertexType
{
	int VertexSize;
	std::vector<VertexAttribute> Attributes;
};

const VertexAttribute *begin(const VertexType &type);

const VertexAttribute *end(const VertexType &type);

extern VertexType vtStandard;

extern VertexType vt2TCoords;

extern VertexType vtTangents;

const VertexType &getVertexTypeDescription(E_VERTEX_TYPE type);

extern VertexType vt2DImage;

extern VertexType vtPrimitive;
}
}
