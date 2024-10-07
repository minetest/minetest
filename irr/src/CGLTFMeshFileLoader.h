// Minetest
// SPDX-License-Identifier: LGPL-2.1-or-later

#pragma once

#include "CSkinnedMesh.h"
#include "IMeshLoader.h"
#include "IReadFile.h"
#include "irrTypes.h"
#include "path.h"
#include "S3DVertex.h"

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
	CGLTFMeshFileLoader() noexcept {};

	bool isALoadableFileExtension(const io::path& filename) const override;

	IAnimatedMesh* createMesh(io::IReadFile* file) override;

private:
	template <typename T>
	static T rawget(const char *ptr);

	template <class T>
	class Accessor
	{
		struct BufferSource
		{
			const char *ptr;
			std::size_t byteStride;
		};
		using Source = std::variant<BufferSource, std::vector<T>, std::tuple<>>;

	public:
		static Accessor sparseIndices(
				const tiniergltf::GlTF &model,
				const tiniergltf::AccessorSparseIndices &indices,
				const std::size_t count);
		static Accessor sparseValues(
				const tiniergltf::GlTF &model,
				const tiniergltf::AccessorSparseValues &values,
				const std::size_t count,
				const std::size_t defaultByteStride);
		static Accessor base(
				const tiniergltf::GlTF &model,
				std::size_t accessorIdx);
		static Accessor make(const tiniergltf::GlTF &model, std::size_t accessorIdx);
		static constexpr tiniergltf::Accessor::Type getType();
		static constexpr tiniergltf::Accessor::ComponentType getComponentType();
		std::size_t getCount() const { return count; }
		T get(std::size_t i) const;

	private:
		Accessor(const char *ptr, std::size_t byteStride, std::size_t count) :
				source(BufferSource{ptr, byteStride}), count(count) {}
		Accessor(std::vector<T> vec, std::size_t count) :
				source(vec), count(count) {}
		Accessor(std::size_t count) :
				source(std::make_tuple()), count(count) {}
		// Directly from buffer, sparse, or default-initialized
		const Source source;
		const std::size_t count;
	};

	template <typename... Ts>
	using AccessorVariant = std::variant<Accessor<Ts>...>;

	template <std::size_t N, typename... Ts>
	using ArrayAccessorVariant = std::variant<Accessor<std::array<Ts, N>>...>;

	template <std::size_t N>
	using NormalizedValuesAccessor = ArrayAccessorVariant<N, u8, u16, f32>;

	template <std::size_t N>
	static NormalizedValuesAccessor<N> createNormalizedValuesAccessor(
			const tiniergltf::GlTF &model,
			const std::size_t accessorIdx);

	template <std::size_t N>
	static std::array<f32, N> getNormalizedValues(
			const NormalizedValuesAccessor<N> &accessor,
			const std::size_t i);

	class MeshExtractor {
	public:
		MeshExtractor(tiniergltf::GlTF &&model,
				CSkinnedMesh *mesh) noexcept
			: m_gltf_model(model), m_irr_model(mesh) {};

		/* Gets indices for the given mesh/primitive.
		 *
		 * Values are return in Irrlicht winding order.
		 */
		std::optional<std::vector<u16>> getIndices(
				const tiniergltf::MeshPrimitive &primitive) const;

		std::optional<std::vector<video::S3DVertex>> getVertices(
				const tiniergltf::MeshPrimitive &primitive) const;

		std::size_t getMeshCount() const;

		std::size_t getPrimitiveCount(const std::size_t meshIdx) const;

		void loadNodes() const;

	private:
		const tiniergltf::GlTF m_gltf_model;
		CSkinnedMesh *m_irr_model;

		void copyPositions(const std::size_t accessorIdx,
				std::vector<video::S3DVertex>& vertices) const;

		void copyNormals(const std::size_t accessorIdx,
				std::vector<video::S3DVertex>& vertices) const;

		void copyTCoords(const std::size_t accessorIdx,
				std::vector<video::S3DVertex>& vertices) const;

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

