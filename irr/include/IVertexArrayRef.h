#pragma once

#include "IReferenceCounted.h"
#include "S3DVertex.h"
#include "SVertexIndex.h"
#include "EHardwareBufferFlags.h"
#include "EPrimitiveTypes.h"
#include "SMaterial.h"
#include <cassert>

namespace irr
{

namespace video
{
	class IVideoDriver;
}

namespace scene
{

class IVertexArrayRef : public virtual IReferenceCounted
{
public:
	IVertexArrayRef(video::E_VERTEX_TYPE vertexType = video::EVT_STANDARD,
		scene::E_HARDWARE_MAPPING mappingHintVertex = scene::EHM_STATIC,
		video::E_INDEX_TYPE indexType = video::EIT_16BIT,
		scene::E_HARDWARE_MAPPING mappingHintIndex = scene::EHM_STATIC)
		  : VertexArrayId(0), VertexBufferId(0), IndexBufferId(0),
			VertexType(vertexType), IndexType(indexType)
	{
		assert(mappingHintVertex != scene::EHM_NEVER);
		MappingHint_Vertex = mappingHintVertex;

		assert(mappingHintIndex != scene::EHM_NEVER);
		MappingHint_Index = mappingHintIndex;
	}

	virtual ~IVertexArrayRef() {}

	virtual void uploadData(u32 vertexCount, const void *vertexData,
		u32 indexCount=0, const void *indexData=nullptr) = 0;

	virtual void bind() const = 0;

	virtual void unbind() const = 0;

	virtual void setAttributes() = 0;

	virtual void draw(video::IVideoDriver *driver, const video::SMaterial &last_material,
		scene::E_PRIMITIVE_TYPE primitive_type = scene::EPT_TRIANGLES, const void* index_data=0) const  = 0;

	bool operator==(const IVertexArrayRef *other_array)
	{
		return (this->VertexArrayId == other_array->VertexArrayId);
	}

protected:
	u32 getPrimitiveCount(scene::E_PRIMITIVE_TYPE primitive_type) const {
		switch (primitive_type) {
		case scene::EPT_POINTS:
			return IndexCount;
		case scene::EPT_LINE_STRIP:
			return IndexCount - 1;
		case scene::EPT_LINE_LOOP:
			return IndexCount;
		case scene::EPT_LINES:
			return IndexCount / 2;
		case scene::EPT_TRIANGLE_STRIP:
			return (IndexCount - 2);
		case scene::EPT_TRIANGLE_FAN:
			return (IndexCount - 2);
		case scene::EPT_TRIANGLES:
			return IndexCount / 3;
		case scene::EPT_POINT_SPRITES:
			return IndexCount;
		}
		return 0;
	}

	u32 VertexArrayId;
	u32 VertexBufferId;
	u32 IndexBufferId;

	video::E_VERTEX_TYPE VertexType;
	video::E_INDEX_TYPE IndexType;

	E_HARDWARE_MAPPING MappingHint_Vertex;
	E_HARDWARE_MAPPING MappingHint_Index;

	u32 VertexCount;
	u32 IndexCount;
};

} // end namespace scene
} // end namespace scene
