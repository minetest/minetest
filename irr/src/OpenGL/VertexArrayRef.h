#pragma once

#include "IVertexArrayRef.h"

namespace irr
{
namespace scene
{

class COpenGL3VertexArrayRef : public IVertexArrayRef
{
public:
	COpenGL3VertexArrayRef();

	~COpenGL3VertexArrayRef();

    void uploadData(u32 vertexCount, const void *vertexData,
		u32 indexCount=0, const void *indexData=nullptr) override;

	void bind() const override;

	void unbind() const override;

	void setAttributes() override;

	void draw(video::IVideoDriver *driver, const video::SMaterial &last_material,
		scene::E_PRIMITIVE_TYPE primitive_type = scene::EPT_TRIANGLES, const void* index_data=0) const override;
};

} // end namespace scene
} // end namespace irr
