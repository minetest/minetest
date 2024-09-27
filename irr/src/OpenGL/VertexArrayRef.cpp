#include "OpenGL/VertexArrayRef.h"
#include "OpenGL/VertexType.h"
#include "vendor/gl.h"
#include "mt_opengl.h"
#include "os.h"

namespace irr
{
namespace scene
{

inline GLenum to_gl_usage(E_HARDWARE_MAPPING mapping)
{
	GLenum usage = GL_STATIC_DRAW;
	if (mapping == scene::EHM_STREAM)
		usage = GL_STREAM_DRAW;
	else if (mapping == scene::EHM_DYNAMIC)
		usage = GL_DYNAMIC_DRAW;

	return usage;
}

inline GLenum to_gl_indextype(video::E_INDEX_TYPE index_type)
{
	GLenum indexType = 0;

	switch (index_type) {
    case (video::EIT_16BIT): {
		indexType = GL_UNSIGNED_SHORT;
		break;
	}
    case (video::EIT_32BIT): {
		indexType = GL_UNSIGNED_INT;
		break;
	}
	}

	return indexType;
}

COpenGL3VertexArrayRef::COpenGL3VertexArrayRef()
	: IVertexArrayRef()
{
	GL.GenVertexArrays(1, &VertexArrayId);
	GL.GenBuffers(1, &VertexBufferId);
}

COpenGL3VertexArrayRef::~COpenGL3VertexArrayRef()
{
	GL.DeleteVertexArrays(1, &VertexArrayId);
	GL.DeleteBuffers(1, &VertexBufferId);

	if (IndexBufferId != 0)
		GL.DeleteBuffers(1, &IndexBufferId);
}

void COpenGL3VertexArrayRef::uploadData(u32 vertexCount, const void *vertexData,
	u32 indexCount, const void *indexData)
{
	bool vdata_valid = vertexCount > 0 && vertexData != nullptr;
	bool idata_valid = indexCount > 0 && indexData != nullptr;

	if (!vdata_valid && !idata_valid) {
		os::Printer::log("Failed to upload the data in the vertex and index buffer (either vertices nor indices provided)");
		return;
	}

	os::Printer::log("uploadData() 1");
	GL.BindVertexArray(VertexArrayId);
	os::Printer::log("uploadData() 2");
	if (vdata_valid) {
		VertexCount = vertexCount;

		GL.BindBuffer(GL_ARRAY_BUFFER, VertexBufferId);

		size_t bufferSize = VertexCount * video::getVertexPitchFromType(VertexType);

		GL.BufferData(GL_ARRAY_BUFFER, bufferSize, vertexData, to_gl_usage(MappingHint_Vertex));
	}
	os::Printer::log("uploadData() 3");

	if (VertexCount > 0 && idata_valid) {
		if (IndexBufferId == 0)
			GL.GenBuffers(1, &IndexBufferId);

		IndexCount = indexCount;

		GL.BindBuffer(GL_ELEMENT_ARRAY_BUFFER, IndexBufferId);

		size_t bufferSize = IndexCount * video::getIndexPitchFromType(IndexType);

		GL.BufferData(GL_ELEMENT_ARRAY_BUFFER, bufferSize, indexData, to_gl_usage(MappingHint_Index));
	}
	os::Printer::log("uploadData() 4");

	setAttributes();
	os::Printer::log("uploadData() 5");

	GL.BindVertexArray(0);
	os::Printer::log("uploadData() 6");
}

void COpenGL3VertexArrayRef::bind() const
{
	GL.BindVertexArray(VertexArrayId);
}

void COpenGL3VertexArrayRef::unbind() const
{
	GL.BindVertexArray(0);
}

void COpenGL3VertexArrayRef::setAttributes()
{
	auto &vertexAttribs = video::getVertexTypeDescription(VertexType);
	for (auto &attr : vertexAttribs.Attributes) {
		GL.EnableVertexAttribArray(attr.Index);
		switch (attr.mode) {
		case video::VertexAttribute::Mode::Regular:
			GL.VertexAttribPointer(attr.Index, attr.ComponentCount, (u32)attr.ComponentType, GL_FALSE, vertexAttribs.VertexSize, (void*)attr.Offset);
			break;
		case video::VertexAttribute::Mode::Normalized:
			GL.VertexAttribPointer(attr.Index, attr.ComponentCount, (u32)attr.ComponentType, GL_TRUE, vertexAttribs.VertexSize, (void*)attr.Offset);
			break;
		case video::VertexAttribute::Mode::Integral:
			GL.VertexAttribIPointer(attr.Index, attr.ComponentCount, (u32)attr.ComponentType, vertexAttribs.VertexSize, (void*)attr.Offset);
			break;
		}
	}
}

void COpenGL3VertexArrayRef::draw(video::IVideoDriver *driver, const video::SMaterial &last_material,
	scene::E_PRIMITIVE_TYPE primitive_type, const void* index_data) const
{
	if (VertexCount == 0 || IndexCount == 0) {
		os::Printer::log("Failed to draw the VAO (the vertex or index data wasn't loaded)");
		return;
	}

	GLenum indexType = to_gl_indextype(IndexType);

	u32 primitiveCount = getPrimitiveCount(primitive_type);

	std::string log1 = "VertexCount " + std::to_string(VertexCount);
	os::Printer::log(log1.c_str());
	std::string log2 = "IndexCount " + std::to_string(IndexCount);
	os::Printer::log(log2.c_str());

	switch (primitive_type) {
	case scene::EPT_POINTS:
	case scene::EPT_POINT_SPRITES:
		GL.DrawArrays(GL_POINTS, 0, primitiveCount);
		break;
	case scene::EPT_LINE_STRIP:
		GL.DrawElements(GL_LINE_STRIP, primitiveCount + 1, indexType, index_data);
		break;
	case scene::EPT_LINE_LOOP:
		GL.DrawElements(GL_LINE_LOOP, primitiveCount, indexType, index_data);
		break;
	case scene::EPT_LINES:
		GL.DrawElements(GL_LINES, primitiveCount * 2, indexType, index_data);
		break;
	case scene::EPT_TRIANGLE_STRIP:
		GL.DrawElements(GL_TRIANGLE_STRIP, primitiveCount + 2, indexType, index_data);
		break;
	case scene::EPT_TRIANGLE_FAN:
		GL.DrawElements(GL_TRIANGLE_FAN, primitiveCount + 2, indexType, index_data);
		break;
	case scene::EPT_TRIANGLES:
		GL.DrawElements((last_material.Wireframe) ? GL_LINES : (last_material.PointCloud) ? GL_POINTS
			: GL_TRIANGLES, primitiveCount * 3, indexType, index_data);
		break;
	default:
		break;
	}
}

} // end namespace scene
} // end namespace irr
