#pragma once

#include "ISkinnedMesh.h"
#include "IMeshLoader.h"
#include "IReadFile.h"
#include "irrTypes.h"
#include "path.h"
#include "S3DVertex.h"
#include "vector2d.h"
#include "vector3d.h"

#include <tiniergltf.hpp>

#include <cstddef>
#include <vector>

namespace irr
{

namespace scene
{

class CGLTFMeshFileLoader : public IMeshLoader
{
public:
	CGLTFMeshFileLoader() noexcept;

	bool isALoadableFileExtension(const io::path& filename) const override;

	IAnimatedMesh* createMesh(io::IReadFile* file) override;

private:
	class BufferOffset
	{
	public:
		BufferOffset(const std::vector<unsigned char>& buf,
				const std::size_t offset);

		BufferOffset(const BufferOffset& other,
				const std::size_t fromOffset);

		unsigned char at(const std::size_t fromOffset) const;
	private:
		const std::vector<unsigned char>& m_buf;
		std::size_t m_offset;
	};

	class MeshExtractor {
	public:
		using vertex_t = video::S3DVertex;

		MeshExtractor(const tiniergltf::GlTF &model,
				ISkinnedMesh *mesh) noexcept
			: m_gltf_model(model), m_irr_model(mesh) {};

		MeshExtractor(tiniergltf::GlTF &&model,
				ISkinnedMesh *mesh) noexcept
			: m_gltf_model(model), m_irr_model(mesh) {};

		/* Gets indices for the given mesh/primitive.
		 *
		 * Values are return in Irrlicht winding order.
		 */
		std::optional<std::vector<u16>> getIndices(const std::size_t meshIdx,
				const std::size_t primitiveIdx) const;

		std::optional<std::vector<vertex_t>> getVertices(std::size_t meshIdx,
				const std::size_t primitiveIdx) const;

		std::size_t getMeshCount() const;

		std::size_t getPrimitiveCount(const std::size_t meshIdx) const;

		void loadNodes() const;

	private:
		const tiniergltf::GlTF m_gltf_model;
		ISkinnedMesh *m_irr_model;

		template <typename T>
		static T readPrimitive(const BufferOffset& readFrom);

		static core::vector2df readVec2DF(
				const BufferOffset& readFrom);

		/* Read a vec3df from a buffer with transformations applied.
		 *
		 * Values are returned in Irrlicht coordinates.
		 */
		static core::vector3df readVec3DF(
				const BufferOffset& readFrom,
				const core::vector3df scale);

		void copyPositions(const std::size_t accessorIdx,
				std::vector<vertex_t>& vertices) const;

		void copyNormals(const std::size_t accessorIdx,
				std::vector<vertex_t>& vertices) const;

		void copyTCoords(const std::size_t accessorIdx,
				std::vector<vertex_t>& vertices) const;

		std::size_t getElemCount(const std::size_t accessorIdx) const;

		std::size_t getByteStride(const std::size_t accessorIdx) const;

		BufferOffset getBuffer(const std::size_t accessorIdx) const;

		std::optional<std::size_t> getIndicesAccessorIdx(const std::size_t meshIdx,
				const std::size_t primitiveIdx) const;

		std::optional<std::size_t> getPositionAccessorIdx(const std::size_t meshIdx,
				const std::size_t primitiveIdx) const;

		/* Get the accessor id of the normals of a primitive.
		 */
		std::optional<std::size_t> getNormalAccessorIdx(const std::size_t meshIdx,
				const std::size_t primitiveIdx) const;

		/* Get the accessor id for the tcoords of a primitive.
		 */
		std::optional<std::size_t> getTCoordAccessorIdx(const std::size_t meshIdx,
				const std::size_t primitiveIdx) const;
		
		void loadMesh(
			std::size_t meshIdx,
			ISkinnedMesh::SJoint *parentJoint) const;

		void loadNode(
			const std::size_t nodeIdx,
			ISkinnedMesh::SJoint *parentJoint) const;
	};

	std::optional<tiniergltf::GlTF> tryParseGLTF(io::IReadFile* file);
};

} // namespace scene

} // namespace irr

