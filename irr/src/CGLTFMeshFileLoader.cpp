#include "CGLTFMeshFileLoader.h"

#include "coreutil.h"
#include "CSkinnedMesh.h"
#include "ISkinnedMesh.h"
#include "irrTypes.h"
#include "IReadFile.h"
#include "matrix4.h"
#include "path.h"
#include "S3DVertex.h"
#include "quaternion.h"
#include "vector3d.h"

#include "tiniergltf.hpp"

#include <array>
#include <cstddef>
#include <cstring>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

namespace irr {

/* Notes on the coordinate system.
 *
 * glTF uses a right-handed coordinate system where +Z is the
 * front-facing axis, and Irrlicht uses a left-handed coordinate
 * system where -Z is the front-facing axis.
 * We convert between them by reflecting the mesh across the X axis.
 * Doing this correctly requires negating the Z coordinate on
 * vertex positions and normals, and reversing the winding order
 * of the vertex indices.
 */

// Right-to-left handedness conversions

template <typename T>
static inline T convertHandedness(const T &t);

template <>
core::vector3df convertHandedness(const core::vector3df &p)
{
	return core::vector3df(p.X, p.Y, -p.Z);
}

namespace scene {

using CMFL = CGLTFMeshFileLoader;

template <class T>
CMFL::Accessor<T>
CMFL::Accessor<T>::sparseIndices(const tiniergltf::GlTF &model,
		const tiniergltf::AccessorSparseIndices &indices,
		const std::size_t count)
{
	const auto &view = model.bufferViews->at(indices.bufferView);
	const auto byteStride = view.byteStride.value_or(indices.elementSize());

	const auto &buffer = model.buffers->at(view.buffer);
	const auto source = buffer.data.data() + view.byteOffset + indices.byteOffset;

	return CMFL::Accessor<T>(source, byteStride, count);
}

template <class T>
CMFL::Accessor<T>
CMFL::Accessor<T>::sparseValues(const tiniergltf::GlTF &model,
		const tiniergltf::AccessorSparseValues &values,
		const std::size_t count,
		const std::size_t defaultByteStride)
{
	const auto &view = model.bufferViews->at(values.bufferView);
	const auto byteStride = view.byteStride.value_or(defaultByteStride);

	const auto &buffer = model.buffers->at(view.buffer);
	const auto source = buffer.data.data() + view.byteOffset + values.byteOffset;

	return CMFL::Accessor<T>(source, byteStride, count);
}

template <class T>
CMFL::Accessor<T>
CMFL::Accessor<T>::base(const tiniergltf::GlTF &model, std::size_t accessorIdx)
{
	const auto &accessor = model.accessors->at(accessorIdx);

	if (!accessor.bufferView.has_value()) {
		return Accessor<T>(accessor.count);
	}

	const auto &view = model.bufferViews->at(accessor.bufferView.value());
	const auto byteStride = view.byteStride.value_or(accessor.elementSize());

	const auto &buffer = model.buffers->at(view.buffer);
	const auto source = buffer.data.data() + view.byteOffset + accessor.byteOffset;

	return Accessor<T>(source, byteStride, accessor.count);
}

template <class T>
CMFL::Accessor<T>
CMFL::Accessor<T>::make(const tiniergltf::GlTF &model, std::size_t accessorIdx)
{
	const auto &accessor = model.accessors->at(accessorIdx);
	if (accessor.componentType != getComponentType() || accessor.type != getType())
		throw std::runtime_error("invalid accessor");

	const auto base = Accessor<T>::base(model, accessorIdx);

	if (accessor.sparse.has_value()) {
		std::vector<T> vec(accessor.count);
		for (std::size_t i = 0; i < accessor.count; ++i) {
			vec[i] = base.get(i);
		}
		const auto overriddenCount = accessor.sparse->count;
		const auto indicesAccessor = ([&]() -> AccessorVariant<u8, u16, u32> {
			switch (accessor.sparse->indices.componentType) {
			case tiniergltf::AccessorSparseIndices::ComponentType::UNSIGNED_BYTE:
				return Accessor<u8>::sparseIndices(model, accessor.sparse->indices, overriddenCount);
			case tiniergltf::AccessorSparseIndices::ComponentType::UNSIGNED_SHORT:
				return Accessor<u16>::sparseIndices(model, accessor.sparse->indices, overriddenCount);
			case tiniergltf::AccessorSparseIndices::ComponentType::UNSIGNED_INT:
				return Accessor<u32>::sparseIndices(model, accessor.sparse->indices, overriddenCount);
			}
			throw std::logic_error("invalid enum value");
		})();

		const auto valuesAccessor = Accessor<T>::sparseValues(model,
				accessor.sparse->values, overriddenCount,
				accessor.bufferView.has_value()
						? model.bufferViews->at(*accessor.bufferView).byteStride.value_or(accessor.elementSize())
						: accessor.elementSize());

		for (std::size_t i = 0; i < overriddenCount; ++i) {
			u32 index;
			std::visit([&](auto &&acc) { index = acc.get(i); }, indicesAccessor);
			if (index >= accessor.count)
				throw std::runtime_error("index out of bounds");
			vec[index] = valuesAccessor.get(i);
		}
		return Accessor<T>(vec, accessor.count);
	}

	return base;
}

#define ACCESSOR_TYPES(T, U, V)                                                         \
	template <>                                                                         \
	constexpr tiniergltf::Accessor::Type CMFL::Accessor<T>::getType()                   \
	{                                                                                   \
		return tiniergltf::Accessor::Type::U;                                           \
	}                                                                                   \
	template <>                                                                         \
	constexpr tiniergltf::Accessor::ComponentType CMFL::Accessor<T>::getComponentType() \
	{                                                                                   \
		return tiniergltf::Accessor::ComponentType::V;                                  \
	}

#define VEC_ACCESSOR_TYPES(T, U, n)                                                                    \
	template <>                                                                                        \
	constexpr tiniergltf::Accessor::Type CMFL::Accessor<std::array<T, n>>::getType()                   \
	{                                                                                                  \
		return tiniergltf::Accessor::Type::VEC##n;                                                     \
	}                                                                                                  \
	template <>                                                                                        \
	constexpr tiniergltf::Accessor::ComponentType CMFL::Accessor<std::array<T, n>>::getComponentType() \
	{                                                                                                  \
		return tiniergltf::Accessor::ComponentType::U;                                                 \
	}                                                                                                  \
	template <>                                                                                        \
	std::array<T, n> CMFL::rawget(const void *ptr)                                                     \
	{                                                                                                  \
		const T *tptr = reinterpret_cast<const T *>(ptr);                                              \
		std::array<T, n> res;                                                                          \
		for (u8 i = 0; i < n; ++i)                                                                     \
			res[i] = rawget<T>(tptr + i);                                                              \
		return res;                                                                                    \
	}

#define ACCESSOR_PRIMITIVE(T, U) \
	ACCESSOR_TYPES(T, SCALAR, U) \
	VEC_ACCESSOR_TYPES(T, U, 2)  \
	VEC_ACCESSOR_TYPES(T, U, 3)  \
	VEC_ACCESSOR_TYPES(T, U, 4)

ACCESSOR_PRIMITIVE(f32, FLOAT)
ACCESSOR_PRIMITIVE(u8, UNSIGNED_BYTE)
ACCESSOR_PRIMITIVE(u16, UNSIGNED_SHORT)
ACCESSOR_PRIMITIVE(u32, UNSIGNED_INT)

ACCESSOR_TYPES(core::vector3df, VEC3, FLOAT)

template <class T>
T CMFL::Accessor<T>::get(std::size_t i) const
{
	// Buffer-based accessor: Read directly from the buffer.
	if (std::holds_alternative<BufferSource>(source)) {
		const auto bufsrc = std::get<BufferSource>(source);
		return rawget<T>(bufsrc.ptr + i * bufsrc.byteStride);
	}
	// Array-based accessor (used for sparse accessors): Read from array.
	if (std::holds_alternative<std::vector<T>>(source)) {
		return std::get<std::vector<T>>(source)[i];
	}
	// Default-initialized accessor.
	// We differ slightly from glTF here in that
	// we default-initialize quaternions and matrices properly,
	// but this does not cause any discrepancies for valid glTF models.
	std::get<std::tuple<>>(source);
	return T();
}

// Note: clang and gcc should both optimize this out.
static inline bool isBigEndian()
{
	const u16 x = 0xFF00;
	return *(const u8 *)(&x);
}

template <typename T>
T CMFL::rawget(const void *ptr)
{
	if (!isBigEndian())
		return *reinterpret_cast<const T *>(ptr);
	// glTF uses little endian.
	// On big-endian systems, we have to swap the byte order.
	// TODO test this on a big endian system
	const u8 *bptr = reinterpret_cast<const u8 *>(ptr);
	u8 bytes[sizeof(T)];
	for (std::size_t i = 0; i < sizeof(T); ++i) {
		bytes[sizeof(T) - i - 1] = bptr[i];
	}
	return *reinterpret_cast<const T *>(bytes);
}

// Note that these "more specialized templates" should win.

template <>
core::matrix4 CMFL::rawget(const void *ptr)
{
	const f32 *fptr = reinterpret_cast<const f32 *>(ptr);
	f32 M[16];
	for (u8 i = 0; i < 16; ++i) {
		M[i] = rawget<f32>(fptr + i);
	}
	core::matrix4 mat;
	mat.setM(M);
	return mat;
}

template <>
core::vector3df CMFL::rawget(const void *ptr)
{
	const f32 *fptr = reinterpret_cast<const f32 *>(ptr);
	return core::vector3df(
			rawget<f32>(fptr),
			rawget<f32>(fptr + 1),
			rawget<f32>(fptr + 2));
}

template <>
core::quaternion CMFL::rawget(const void *ptr)
{
	const f32 *fptr = reinterpret_cast<const f32 *>(ptr);
	return core::quaternion(
			rawget<f32>(fptr),
			rawget<f32>(fptr + 1),
			rawget<f32>(fptr + 2),
			rawget<f32>(fptr + 3));
}

template <std::size_t N>
CMFL::NormalizedValuesAccessor<N>
CMFL::createNormalizedValuesAccessor(
		const tiniergltf::GlTF &model,
		const std::size_t accessorIdx)
{
	const auto &acc = model.accessors->at(accessorIdx);
	switch (acc.componentType) {
	case tiniergltf::Accessor::ComponentType::UNSIGNED_BYTE:
		return Accessor<std::array<u8, N>>::make(model, accessorIdx);
	case tiniergltf::Accessor::ComponentType::UNSIGNED_SHORT:
		return Accessor<std::array<u16, N>>::make(model, accessorIdx);
	case tiniergltf::Accessor::ComponentType::FLOAT:
		return Accessor<std::array<f32, N>>::make(model, accessorIdx);
	default:
		throw std::runtime_error("invalid component type");
	}
}

template <std::size_t N>
std::array<f32, N> CMFL::getNormalizedValues(
		const NormalizedValuesAccessor<N> &accessor,
		const std::size_t i)
{
	std::array<f32, N> values;
	if (std::holds_alternative<Accessor<std::array<u8, N>>>(accessor)) {
		const auto u8s = std::get<Accessor<std::array<u8, N>>>(accessor).get(i);
		for (u8 i = 0; i < N; ++i)
			values[i] = static_cast<f32>(u8s[i]) / std::numeric_limits<u8>::max();
	} else if (std::holds_alternative<Accessor<std::array<u16, N>>>(accessor)) {
		const auto u16s = std::get<Accessor<std::array<u16, N>>>(accessor).get(i);
		for (u8 i = 0; i < N; ++i)
			values[i] = static_cast<f32>(u16s[i]) / std::numeric_limits<u16>::max();
	} else {
		values = std::get<Accessor<std::array<f32, N>>>(accessor).get(i);
		for (u8 i = 0; i < N; ++i) {
			if (values[i] < 0 || values[i] > 1)
				throw std::runtime_error("invalid normalized value");
		}
	}
	return values;
}

CGLTFMeshFileLoader::CGLTFMeshFileLoader() noexcept
{
}

/**
 * The most basic portion of the code base. This tells irllicht if this file has a .gltf extension.
*/
bool CGLTFMeshFileLoader::isALoadableFileExtension(
		const io::path& filename) const
{
	return core::hasFileExtension(filename, "gltf");
}

/**
 * Entry point into loading a GLTF model.
*/
IAnimatedMesh* CGLTFMeshFileLoader::createMesh(io::IReadFile* file)
{
	if (file->getSize() <= 0) {
		return nullptr;
	}
	std::optional<tiniergltf::GlTF> model = tryParseGLTF(file);
	if (!model.has_value()) {
		return nullptr;
	}

	if (!(model->buffers.has_value()
			&& model->bufferViews.has_value()
			&& model->accessors.has_value()
			&& model->meshes.has_value()
			&& model->nodes.has_value())) {
		return nullptr;
	}

	ISkinnedMesh *mesh = new CSkinnedMesh();
	MeshExtractor parser(std::move(model.value()), mesh);
	try {
		parser.loadNodes();
	} catch (std::runtime_error &e) {
		mesh->drop();
		return nullptr;
	}
	return mesh;
}

static void transformVertices(std::vector<video::S3DVertex> &vertices, const core::matrix4 &transform)
{
	for (auto &vertex : vertices) {
		// Apply scaling, rotation and rotation (in that order) to the position.
		transform.transformVect(vertex.Pos);
		// For the normal, we do not want to apply the translation.
		// TODO note that this also applies scaling; the Irrlicht method is misnamed.
		transform.rotateVect(vertex.Normal);
		// Renormalize (length might have been affected by scaling).
		vertex.Normal.normalize();
	}
}

static void checkIndices(const std::vector<u16> &indices, const std::size_t nVerts)
{
	for (u16 index : indices) {
		if (index >= nVerts)
			throw std::runtime_error("index out of bounds");
	}
}

static std::vector<u16> generateIndices(const std::size_t nVerts)
{
	std::vector<u16> indices(nVerts);
	for (std::size_t i = 0; i < nVerts; i += 3) {
		// Reverse winding order per triangle
		indices[i] = i + 2;
		indices[i + 1] = i + 1;
		indices[i + 2] = i;
	}
	return indices;
}

/**
 * Load up the rawest form of the model. The vertex positions and indices.
 * Documentation: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes
 * If material is undefined, then a default material MUST be used.
*/
void CGLTFMeshFileLoader::MeshExtractor::loadMesh(
		const std::size_t meshIdx,
		ISkinnedMesh::SJoint *parent) const
{
	for (std::size_t j = 0; j < getPrimitiveCount(meshIdx); ++j) {
		auto vertices = getVertices(meshIdx, j);
		if (!vertices.has_value())
			continue; // "When positions are not specified, client implementations SHOULD skip primitive’s rendering"

		// Excludes the max value for consistency.
		if (vertices->size() >= std::numeric_limits<u16>::max())
			throw std::runtime_error("too many vertices");

		// Apply the global transform along the parent chain.
		transformVertices(*vertices, parent->GlobalMatrix);
		
		auto maybeIndices = getIndices(meshIdx, j);
		std::vector<u16> indices;
		if (maybeIndices.has_value()) {
			indices = std::move(*maybeIndices);
			checkIndices(indices, vertices->size());
		} else {
			// Non-indexed geometry
			indices = generateIndices(vertices->size());
		}

		auto *meshbuf = m_irr_model->addMeshBuffer();
		meshbuf->append(vertices->data(), vertices->size(),
			indices.data(), indices.size());
	}
}

// Base transformation between left & right handed coordinate systems.
// This just inverts the Z axis.
static const core::matrix4 leftToRight = core::matrix4(
	1, 0, 0, 0,
	0, 1, 0, 0,
	0, 0, -1, 0,
	0, 0, 0, 1
);
static const core::matrix4 rightToLeft = leftToRight;

static core::matrix4 loadTransform(const tiniergltf::Node::Matrix &m)
{
	// Note: Under the hood, this casts these doubles to floats.
	return core::matrix4(
			m[0], m[1], m[2], m[3],
			m[4], m[5], m[6], m[7],
			m[8], m[9], m[10], m[11],
			m[12], m[13], m[14], m[15]);
}

static core::matrix4 loadTransform(const tiniergltf::Node::TRS &trs)
{
	const auto &trans = trs.translation;
	const auto &rot = trs.rotation;
	const auto &scale = trs.scale;
	core::matrix4 transMat;
	transMat.setTranslation(core::vector3df(trans[0], trans[1], trans[2]));
	core::matrix4 rotMat = core::quaternion(rot[0], rot[1], rot[2], rot[3]).getMatrix();
	core::matrix4 scaleMat;
	scaleMat.setScale(core::vector3df(scale[0], scale[1], scale[2]));
	return transMat * rotMat * scaleMat;
}

static core::matrix4 loadTransform(std::optional<std::variant<tiniergltf::Node::Matrix, tiniergltf::Node::TRS>> transform) {
	if (!transform.has_value()) {
		return core::matrix4();
	}
	core::matrix4 mat = std::visit([](const auto &t) { return loadTransform(t); }, *transform);
	return rightToLeft * mat * leftToRight;
}

void CGLTFMeshFileLoader::MeshExtractor::loadNode(
		const std::size_t nodeIdx,
		ISkinnedMesh::SJoint *parent) const
{
	const auto &node = m_gltf_model.nodes->at(nodeIdx);
	auto *joint = m_irr_model->addJoint(parent);
	const core::matrix4 transform = loadTransform(node.transform);
	joint->LocalMatrix = transform;
	joint->GlobalMatrix = parent ? parent->GlobalMatrix * joint->LocalMatrix : joint->LocalMatrix;
	if (node.name.has_value()) {
		joint->Name = node.name->c_str();
	}
	if (node.mesh.has_value()) {
		loadMesh(*node.mesh, joint);
	}
	if (node.children.has_value()) {
		for (const auto &child : *node.children) {
			loadNode(child, joint);
		}
	}
}

void CGLTFMeshFileLoader::MeshExtractor::loadNodes() const
{
	std::vector<bool> isChild(m_gltf_model.nodes->size());
	for (const auto &node : *m_gltf_model.nodes) {
		if (!node.children.has_value())
			continue;
		for (const auto &child : *node.children) {
			isChild[child] = true;
		}
	}
	// Load all nodes that aren't children.
	// Children will be loaded by their parent nodes.
	for (std::size_t i = 0; i < m_gltf_model.nodes->size(); ++i) {
		if (!isChild[i]) {
			loadNode(i, nullptr);
		}
	}
}

/**
 * Extracts GLTF mesh indices.
 */
std::optional<std::vector<u16>> CMFL::MeshExtractor::getIndices(
		const std::size_t meshIdx,
		const std::size_t primitiveIdx) const
{
	const auto accessorIdx = m_gltf_model.meshes->at(meshIdx).primitives.at(primitiveIdx).indices;
	if (!accessorIdx.has_value())
		return std::nullopt; // non-indexed geometry

	const auto accessor = ([&]() -> AccessorVariant<u8, u16, u32> {
		const auto &acc = m_gltf_model.accessors->at(*accessorIdx);
		switch (acc.componentType) {
		case tiniergltf::Accessor::ComponentType::UNSIGNED_BYTE:
			return Accessor<u8>::make(m_gltf_model, *accessorIdx);
		case tiniergltf::Accessor::ComponentType::UNSIGNED_SHORT:
			return Accessor<u16>::make(m_gltf_model, *accessorIdx);
		case tiniergltf::Accessor::ComponentType::UNSIGNED_INT:
			return Accessor<u32>::make(m_gltf_model, *accessorIdx);
		default:
			throw std::runtime_error("invalid component type");
		}
	})();
	const auto count = std::visit([](auto &&a) { return a.getCount(); }, accessor);

	std::vector<u16> indices;
	for (std::size_t i = 0; i < count; ++i) {
		// TODO (low-priority, maybe never) also reverse winding order based on determinant of global transform
		// FIXME this hack also reverses triangle draw order
		std::size_t elemIdx = count - i - 1; // reverse index order
		u16 index;
		// Note: glTF forbids the max value for each component type.
		if (std::holds_alternative<Accessor<u8>>(accessor)) {
			index = std::get<Accessor<u8>>(accessor).get(elemIdx);
			if (index == std::numeric_limits<u8>::max())
				throw std::runtime_error("invalid index");
		} else if (std::holds_alternative<Accessor<u16>>(accessor)) {
			index = std::get<Accessor<u16>>(accessor).get(elemIdx);
			if (index == std::numeric_limits<u16>::max())
				throw std::runtime_error("invalid index");
		} else if (std::holds_alternative<Accessor<u32>>(accessor)) {
			u32 indexWide = std::get<Accessor<u32>>(accessor).get(elemIdx);
			// Use >= here for consistency.
			if (indexWide >= std::numeric_limits<u16>::max())
				throw std::runtime_error("index too large (>= 65536)");
			index = static_cast<u16>(indexWide);
		}
		indices.push_back(index);
	}

	return indices;
}

/**
 * Create a vector of video::S3DVertex (model data) from a mesh & primitive index.
 */
std::optional<std::vector<video::S3DVertex>> CMFL::MeshExtractor::getVertices(
		const std::size_t meshIdx,
		const std::size_t primitiveIdx) const
{
	const auto &attributes = m_gltf_model.meshes->at(meshIdx).primitives.at(primitiveIdx).attributes;
	const auto positionAccessorIdx = attributes.position;
	if (!positionAccessorIdx.has_value()) {
		// "When positions are not specified, client implementations SHOULD skip primitive's rendering"
		return std::nullopt;
	}

	std::vector<video::S3DVertex> vertices;
	const auto vertexCount = m_gltf_model.accessors->at(*positionAccessorIdx).count;
	vertices.resize(vertexCount);
	copyPositions(*positionAccessorIdx, vertices);

	const auto normalAccessorIdx = attributes.normal;
	if (normalAccessorIdx.has_value()) {
		copyNormals(normalAccessorIdx.value(), vertices);
	}
	// TODO verify that the automatic normal recalculation done in Minetest indeed works correctly

	const auto &texcoords = m_gltf_model.meshes->at(meshIdx).primitives[primitiveIdx].attributes.texcoord;
	if (texcoords.has_value()) {
		const auto tCoordAccessorIdx = texcoords->at(0);
		copyTCoords(tCoordAccessorIdx, vertices);
	}

	return vertices;
}

/**
 * Get the amount of meshes that a model contains.
*/
std::size_t CGLTFMeshFileLoader::MeshExtractor::getMeshCount() const
{
	return m_gltf_model.meshes->size();
}

/**
 * Get the amount of primitives that a mesh in a model contains.
*/
std::size_t CGLTFMeshFileLoader::MeshExtractor::getPrimitiveCount(
		const std::size_t meshIdx) const
{
	return m_gltf_model.meshes->at(meshIdx).primitives.size();
}

/**
 * Streams vertex positions raw data into usable buffer via reference.
 * Buffer: ref Vector<video::S3DVertex>
*/
void CGLTFMeshFileLoader::MeshExtractor::copyPositions(
		const std::size_t accessorIdx,
		std::vector<video::S3DVertex>& vertices) const
{
	const auto accessor = Accessor<core::vector3df>::make(m_gltf_model, accessorIdx);
	for (std::size_t i = 0; i < accessor.getCount(); i++) {
		vertices[i].Pos = convertHandedness(accessor.get(i));
	}
}

/**
 * Streams normals raw data into usable buffer via reference.
 * Buffer: ref Vector<video::S3DVertex>
*/
void CGLTFMeshFileLoader::MeshExtractor::copyNormals(
		const std::size_t accessorIdx,
		std::vector<video::S3DVertex>& vertices) const
{
	const auto accessor = Accessor<core::vector3df>::make(m_gltf_model, accessorIdx);
	for (std::size_t i = 0; i < accessor.getCount(); ++i) {
		vertices[i].Normal = convertHandedness(accessor.get(i));
	}
}

/**
 * Streams texture coordinate raw data into usable buffer via reference.
 * Buffer: ref Vector<video::S3DVertex>
*/
void CGLTFMeshFileLoader::MeshExtractor::copyTCoords(
		const std::size_t accessorIdx,
		std::vector<video::S3DVertex>& vertices) const
{
	const auto accessor = createNormalizedValuesAccessor<2>(m_gltf_model, accessorIdx);
	const auto count = std::visit([](auto &&a) { return a.getCount(); }, accessor);
	for (std::size_t i = 0; i < count; ++i) {
		const auto vals = getNormalizedValues(accessor, i);
		vertices[i].TCoords = core::vector2df(vals[0], vals[1]);
	}
}

/**
 * The index of the accessor that contains the vertex indices. 
 * When this is undefined, the primitive defines non-indexed geometry. 
 * When defined, the accessor MUST have SCALAR type and an unsigned integer component type.
 * Documentation: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#_mesh_primitive_indices
 * Type: Integer
 * Required: NO
*/
std::optional<std::size_t> CGLTFMeshFileLoader::MeshExtractor::getIndicesAccessorIdx(
		const std::size_t meshIdx,
		const std::size_t primitiveIdx) const
{
	return m_gltf_model.meshes->at(meshIdx).primitives[primitiveIdx].indices;
}

/**
 * The index of the accessor that contains the POSITIONs.
 * Documentation: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes-overview
 * Type: VEC3 (Float)
*/
std::optional<std::size_t> CGLTFMeshFileLoader::MeshExtractor::getPositionAccessorIdx(
		const std::size_t meshIdx,
		const std::size_t primitiveIdx) const
{
	return m_gltf_model.meshes->at(meshIdx).primitives[primitiveIdx].attributes.position;
}

/**
 * The index of the accessor that contains the NORMALs.
 * Documentation: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes-overview
 * Type: VEC3 (Float)
 * ! Required: NO (Appears to not be, needs another pair of eyes to research.)
*/
std::optional<std::size_t> CGLTFMeshFileLoader::MeshExtractor::getNormalAccessorIdx(
		const std::size_t meshIdx,
		const std::size_t primitiveIdx) const
{
	return m_gltf_model.meshes->at(meshIdx).primitives[primitiveIdx].attributes.normal;
}

/**
 * The index of the accessor that contains the TEXCOORDs.
 * Documentation: https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html#meshes-overview
 * Type: VEC3 (Float)
 * ! Required: YES (Appears so, needs another pair of eyes to research.)
*/
std::optional<std::size_t> CGLTFMeshFileLoader::MeshExtractor::getTCoordAccessorIdx(
		const std::size_t meshIdx,
		const std::size_t primitiveIdx) const
{
	const auto& texcoords = m_gltf_model.meshes->at(meshIdx).primitives[primitiveIdx].attributes.texcoord;
	if (!texcoords.has_value())
		return std::nullopt;
	return texcoords->at(0);
}

/**
 * This is where the actual model's GLTF file is loaded and parsed by tiniergltf.
*/
std::optional<tiniergltf::GlTF> CGLTFMeshFileLoader::tryParseGLTF(io::IReadFile* file)
{
	auto size = file->getSize();
	auto buf = std::make_unique<char[]>(size + 1);
	file->read(buf.get(), size);
	// We probably don't need this, but add it just to be sure.
	buf[size] = '\0';
	Json::CharReaderBuilder builder;
    const std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
	Json::Value json;
	JSONCPP_STRING err;
    if (!reader->parse(buf.get(), buf.get() + size, &json, &err)) {
      return std::nullopt;
    }
	try {
		return tiniergltf::GlTF(json);
	}  catch (const std::runtime_error &e) {
		return std::nullopt;
	} catch (const std::out_of_range &e) {
		return std::nullopt;
	}
}

} // namespace scene

} // namespace irr

