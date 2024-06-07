// Minetest
// SPDX-License-Identifier: LGPL-2.1-or-later

#ifndef UNITTEST_ASSETS_DIRECTORY
#error "The required definition for UNITTEST_ASSETS_DIRECTORY is missing."
#endif

#include "CReadFile.h"
#include "vector3d.h"

#include <irrlicht.h>

// Catch needs to be included after Irrlicht so that it sees operator<<
// declarations.
#include <catch.hpp>

#include <cstring>

#define XSTR(s) STR(s)
#define STR(s) #s

using namespace std;

class ScopedMesh
{
public:
	ScopedMesh(irr::io::IReadFile* file)
		: m_device { irr::createDevice(irr::video::EDT_NULL) }
		, m_mesh { nullptr }
	{
		auto* smgr = m_device->getSceneManager();
		m_mesh = smgr->getMesh(file);
	}

	ScopedMesh(const irr::io::path& filepath)
		: m_device { irr::createDevice(irr::video::EDT_NULL) }
		, m_mesh { nullptr }
	{
		auto* smgr = m_device->getSceneManager();
		irr::io::CReadFile f = irr::io::CReadFile(filepath);
		m_mesh = smgr->getMesh(&f);
	}

	~ScopedMesh()
	{
		m_device->drop();
		m_mesh = nullptr;
	}

	const irr::scene::IAnimatedMesh* getMesh() const
	{
		return m_mesh;
	}

private:
	irr::IrrlichtDevice* m_device;
	irr::scene::IAnimatedMesh* m_mesh;
};

using v3f = irr::core::vector3df;
using v2f = irr::core::vector2df;

TEST_CASE("load empty gltf file") {
	ScopedMesh sm(XSTR(UNITTEST_ASSETS_DIRECTORY) "empty.gltf");
	CHECK(sm.getMesh() == nullptr);
}

TEST_CASE("minimal triangle") {
	auto path = GENERATE(
			XSTR(UNITTEST_ASSETS_DIRECTORY) "minimal_triangle.gltf",
			XSTR(UNITTEST_ASSETS_DIRECTORY) "triangle_with_vertex_stride.gltf",
			// Test non-indexed geometry.
			XSTR(UNITTEST_ASSETS_DIRECTORY) "triangle_without_indices.gltf");
	INFO(path);
	ScopedMesh sm(path);
	REQUIRE(sm.getMesh() != nullptr);
	REQUIRE(sm.getMesh()->getMeshBufferCount() == 1);

	SECTION("vertex coordinates are correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 3);
		auto vertices = static_cast<const irr::video::S3DVertex *>(
				sm.getMesh()->getMeshBuffer(0)->getVertices());
		CHECK(vertices[0].Pos == v3f {0.0f, 0.0f, 0.0f});
		CHECK(vertices[1].Pos == v3f {1.0f, 0.0f, 0.0f});
		CHECK(vertices[2].Pos == v3f {0.0f, 1.0f, 0.0f});
	}

	SECTION("vertex indices are correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getIndexCount() == 3);
		auto indices = static_cast<const irr::u16 *>(
				sm.getMesh()->getMeshBuffer(0)->getIndices());
		CHECK(indices[0] == 2);
		CHECK(indices[1] == 1);
		CHECK(indices[2] == 0);
	}
}

TEST_CASE("blender cube") {
	ScopedMesh sm(XSTR(UNITTEST_ASSETS_DIRECTORY) "blender_cube.gltf");
	REQUIRE(sm.getMesh() != nullptr);
	REQUIRE(sm.getMesh()->getMeshBufferCount() == 1);
	SECTION("vertex coordinates are correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
		auto vertices = static_cast<const irr::video::S3DVertex *>(
				sm.getMesh()->getMeshBuffer(0)->getVertices());
		CHECK(vertices[0].Pos == v3f{-10.0f, -10.0f, -10.0f});
		CHECK(vertices[3].Pos == v3f{-10.0f, 10.0f, -10.0f});
		CHECK(vertices[6].Pos == v3f{-10.0f, -10.0f, 10.0f});
		CHECK(vertices[9].Pos == v3f{-10.0f, 10.0f, 10.0f});
		CHECK(vertices[12].Pos == v3f{10.0f, -10.0f, -10.0f});
		CHECK(vertices[15].Pos == v3f{10.0f, 10.0f, -10.0f});
		CHECK(vertices[18].Pos == v3f{10.0f, -10.0f, 10.0f});
		CHECK(vertices[21].Pos == v3f{10.0f, 10.0f, 10.0f});
	}

	SECTION("vertex indices are correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getIndexCount() == 36);
		auto indices = static_cast<const irr::u16 *>(
				sm.getMesh()->getMeshBuffer(0)->getIndices());
		CHECK(indices[0] == 16);
		CHECK(indices[1] == 5);
		CHECK(indices[2] == 22);
		CHECK(indices[35] == 0);
	}

	SECTION("vertex normals are correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
		auto vertices = static_cast<const irr::video::S3DVertex *>(
				sm.getMesh()->getMeshBuffer(0)->getVertices());
		CHECK(vertices[0].Normal == v3f{-1.0f, 0.0f, 0.0f});
		CHECK(vertices[1].Normal == v3f{0.0f, -1.0f, 0.0f});
		CHECK(vertices[2].Normal == v3f{0.0f, 0.0f, -1.0f});
		CHECK(vertices[3].Normal == v3f{-1.0f, 0.0f, 0.0f});
		CHECK(vertices[6].Normal == v3f{-1.0f, 0.0f, 0.0f});
		CHECK(vertices[23].Normal == v3f{1.0f, 0.0f, 0.0f});

	}

	SECTION("texture coords are correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
		auto vertices = static_cast<const irr::video::S3DVertex *>(
				sm.getMesh()->getMeshBuffer(0)->getVertices());
		CHECK(vertices[0].TCoords == v2f{0.375f, 1.0f});
		CHECK(vertices[1].TCoords == v2f{0.125f, 0.25f});
		CHECK(vertices[2].TCoords == v2f{0.375f, 0.0f});
		CHECK(vertices[3].TCoords == v2f{0.6250f, 1.0f});
		CHECK(vertices[6].TCoords == v2f{0.375f, 0.75f});
	}
}

TEST_CASE("mesh loader returns nullptr when given null file pointer") {
	ScopedMesh sm(nullptr);
	CHECK(sm.getMesh() == nullptr);
}

TEST_CASE("invalid JSON returns nullptr") {
	ScopedMesh sm(XSTR(UNITTEST_ASSETS_DIRECTORY) "json_missing_brace.gltf");
	CHECK(sm.getMesh() == nullptr);
}

TEST_CASE("blender cube scaled") {
	ScopedMesh sm(XSTR(UNITTEST_ASSETS_DIRECTORY) "blender_cube_scaled.gltf");
	REQUIRE(sm.getMesh() != nullptr);
	REQUIRE(sm.getMesh()->getMeshBufferCount() == 1);
	
	SECTION("Scaling is correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
		auto vertices = static_cast<const irr::video::S3DVertex *>(
				sm.getMesh()->getMeshBuffer(0)->getVertices());

		CHECK(vertices[0].Pos == v3f{-150.0f, -1.0f, -21.5f});
		CHECK(vertices[3].Pos == v3f{-150.0f, 1.0f, -21.5f});
		CHECK(vertices[6].Pos == v3f{-150.0f, -1.0f, 21.5f});
		CHECK(vertices[9].Pos == v3f{-150.0f, 1.0f, 21.5f});
		CHECK(vertices[12].Pos == v3f{150.0f, -1.0f, -21.5f});
		CHECK(vertices[15].Pos == v3f{150.0f, 1.0f, -21.5f});
		CHECK(vertices[18].Pos == v3f{150.0f, -1.0f, 21.5f});
		CHECK(vertices[21].Pos == v3f{150.0f, 1.0f, 21.5f});
	}
}

TEST_CASE("blender cube matrix transform") {
	ScopedMesh sm(XSTR(UNITTEST_ASSETS_DIRECTORY) "blender_cube_matrix_transform.gltf");
	REQUIRE(sm.getMesh() != nullptr);
	REQUIRE(sm.getMesh()->getMeshBufferCount() == 1);
	
	SECTION("Transformation is correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
		auto vertices = static_cast<const irr::video::S3DVertex *>(
				sm.getMesh()->getMeshBuffer(0)->getVertices());
		const auto checkVertex = [&](const std::size_t i, v3f vec) {
			// The transform scales by (1, 2, 3) and translates by (4, 5, 6).
			CHECK(vertices[i].Pos == vec * v3f{1, 2, 3}
					// The -6 is due to the coordinate system conversion.
					+ v3f{4, 5, -6});
		};
		checkVertex(0, v3f{-1, -1, -1});
		checkVertex(3, v3f{-1, 1, -1});
		checkVertex(6, v3f{-1, -1, 1});
		checkVertex(9, v3f{-1, 1, 1});
		checkVertex(12, v3f{1, -1, -1});
		checkVertex(15, v3f{1, 1, -1});
		checkVertex(18, v3f{1, -1, 1});
		checkVertex(21, v3f{1, 1, 1});
	}
}

TEST_CASE("snow man") {
	ScopedMesh sm(XSTR(UNITTEST_ASSETS_DIRECTORY) "snow_man.gltf");
	REQUIRE(sm.getMesh() != nullptr);
	REQUIRE(sm.getMesh()->getMeshBufferCount() == 3);

	SECTION("vertex coordinates are correct for all buffers") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
		{
			auto vertices = static_cast<const irr::video::S3DVertex *>(
					sm.getMesh()->getMeshBuffer(0)->getVertices());
			CHECK(vertices[0].Pos == v3f{3.0f, 24.0f, -3.0f});
			CHECK(vertices[3].Pos == v3f{3.0f, 18.0f, 3.0f});
			CHECK(vertices[6].Pos == v3f{-3.0f, 18.0f, -3.0f});
			CHECK(vertices[9].Pos == v3f{3.0f, 24.0f, 3.0f});
			CHECK(vertices[12].Pos == v3f{3.0f, 18.0f, -3.0f});
			CHECK(vertices[15].Pos == v3f{-3.0f, 18.0f, 3.0f});
			CHECK(vertices[18].Pos == v3f{3.0f, 18.0f, -3.0f});
			CHECK(vertices[21].Pos == v3f{3.0f, 18.0f, 3.0f});
		}
		{
			REQUIRE(sm.getMesh()->getMeshBuffer(1)->getVertexCount() == 24);
			auto vertices = static_cast<const irr::video::S3DVertex *>(
					sm.getMesh()->getMeshBuffer(1)->getVertices());
			CHECK(vertices[2].Pos == v3f{5.0f, 10.0f, 5.0f});
			CHECK(vertices[3].Pos == v3f{5.0f, 0.0f, 5.0f});
			CHECK(vertices[7].Pos == v3f{-5.0f, 0.0f, 5.0f});
			CHECK(vertices[8].Pos == v3f{5.0f, 10.0f, -5.0f});
			CHECK(vertices[14].Pos == v3f{5.0f, 0.0f, 5.0f});
			CHECK(vertices[16].Pos == v3f{5.0f, 10.0f, -5.0f});
			CHECK(vertices[22].Pos == v3f{-5.0f, 10.0f, 5.0f});
			CHECK(vertices[23].Pos == v3f{-5.0f, 0.0f, 5.0f});
		}
		{
			REQUIRE(sm.getMesh()->getMeshBuffer(2)->getVertexCount() == 24);
			auto vertices = static_cast<const irr::video::S3DVertex *>(
					sm.getMesh()->getMeshBuffer(2)->getVertices());
			CHECK(vertices[1].Pos == v3f{4.0f, 10.0f, -4.0f});
			CHECK(vertices[2].Pos == v3f{4.0f, 18.0f, 4.0f});
			CHECK(vertices[3].Pos == v3f{4.0f, 10.0f, 4.0f});
			CHECK(vertices[10].Pos == v3f{-4.0f, 18.0f, -4.0f});
			CHECK(vertices[11].Pos == v3f{-4.0f, 18.0f, 4.0f});
			CHECK(vertices[12].Pos == v3f{4.0f, 10.0f, -4.0f});
			CHECK(vertices[17].Pos == v3f{-4.0f, 18.0f, -4.0f});
			CHECK(vertices[18].Pos == v3f{4.0f, 10.0f, -4.0f});
		}
	}

	SECTION("vertex indices are correct for all buffers") {
		{
			REQUIRE(sm.getMesh()->getMeshBuffer(0)->getIndexCount() == 36);
			auto indices = static_cast<const irr::u16 *>(
					sm.getMesh()->getMeshBuffer(0)->getIndices());
			CHECK(indices[0] == 23);
			CHECK(indices[1] == 21);
			CHECK(indices[2] == 22);
			CHECK(indices[35] == 2);
		}
		{
			REQUIRE(sm.getMesh()->getMeshBuffer(1)->getIndexCount() == 36);
			auto indices = static_cast<const irr::u16 *>(
					sm.getMesh()->getMeshBuffer(1)->getIndices());
			CHECK(indices[10] == 16);
			CHECK(indices[11] == 18);
			CHECK(indices[15] == 13);
			CHECK(indices[27] == 5);
		}
		{
			REQUIRE(sm.getMesh()->getMeshBuffer(2)->getIndexCount() == 36);
			auto indices = static_cast<const irr::u16 *>(
					sm.getMesh()->getMeshBuffer(2)->getIndices());
			CHECK(indices[26] == 6);
			CHECK(indices[27] == 5);
			CHECK(indices[29] == 6);
			CHECK(indices[32] == 2);
		}
	}

	
	SECTION("vertex normals are correct for all buffers") {
		{
			REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
			auto vertices = static_cast<const irr::video::S3DVertex *>(
					sm.getMesh()->getMeshBuffer(0)->getVertices());
			CHECK(vertices[0].Normal == v3f{1.0f, 0.0f, -0.0f});
			CHECK(vertices[1].Normal == v3f{1.0f, 0.0f, -0.0f});
			CHECK(vertices[2].Normal == v3f{1.0f, 0.0f, -0.0f});
			CHECK(vertices[3].Normal == v3f{1.0f, 0.0f, -0.0f});
			CHECK(vertices[6].Normal == v3f{-1.0f, 0.0f, -0.0f});
			CHECK(vertices[23].Normal == v3f{0.0f, 0.0f, 1.0f});
		}
		{
			REQUIRE(sm.getMesh()->getMeshBuffer(1)->getVertexCount() == 24);
			auto vertices = static_cast<const irr::video::S3DVertex *>(
					sm.getMesh()->getMeshBuffer(1)->getVertices());
			CHECK(vertices[0].Normal == v3f{1.0f, 0.0f, -0.0f});
			CHECK(vertices[1].Normal == v3f{1.0f, 0.0f, -0.0f});
			CHECK(vertices[3].Normal == v3f{1.0f, 0.0f, -0.0f});
			CHECK(vertices[6].Normal == v3f{-1.0f, 0.0f, -0.0f});
			CHECK(vertices[7].Normal == v3f{-1.0f, 0.0f, -0.0f});
			CHECK(vertices[22].Normal == v3f{0.0f, 0.0f, 1.0f});
		}
		{
			REQUIRE(sm.getMesh()->getMeshBuffer(2)->getVertexCount() == 24);
			auto vertices = static_cast<const irr::video::S3DVertex *>(
				sm.getMesh()->getMeshBuffer(2)->getVertices());
			CHECK(vertices[3].Normal == v3f{1.0f, 0.0f, -0.0f});
			CHECK(vertices[4].Normal == v3f{-1.0f, 0.0f, -0.0f});
			CHECK(vertices[5].Normal == v3f{-1.0f, 0.0f, -0.0f});
			CHECK(vertices[10].Normal == v3f{0.0f, 1.0f, -0.0f});
			CHECK(vertices[11].Normal == v3f{0.0f, 1.0f, -0.0f});
			CHECK(vertices[19].Normal == v3f{0.0f, 0.0f, -1.0f});
		}
	}

	
	SECTION("texture coords are correct for all buffers") {
		{
			REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
			auto vertices = static_cast<const irr::video::S3DVertex *>(
					sm.getMesh()->getMeshBuffer(0)->getVertices());
			CHECK(vertices[0].TCoords == v2f{0.583333313f, 0.791666686f});
			CHECK(vertices[1].TCoords == v2f{0.583333313f, 0.666666686f});
			CHECK(vertices[2].TCoords == v2f{0.708333313f, 0.791666686f});
			CHECK(vertices[5].TCoords == v2f{0.375f, 0.416666657f});
			CHECK(vertices[6].TCoords == v2f{0.5f, 0.291666657f});
			CHECK(vertices[19].TCoords == v2f{0.708333313f, 0.75f});
		}
		{
			REQUIRE(sm.getMesh()->getMeshBuffer(1)->getVertexCount() == 24);
			auto vertices = static_cast<const irr::video::S3DVertex *>(
					sm.getMesh()->getMeshBuffer(1)->getVertices());

			CHECK(vertices[1].TCoords == v2f{0.0f, 0.791666686f});
			CHECK(vertices[4].TCoords == v2f{0.208333328f, 0.791666686f});
			CHECK(vertices[5].TCoords == v2f{0.0f, 0.791666686f});
			CHECK(vertices[6].TCoords == v2f{0.208333328f, 0.583333313f});
			CHECK(vertices[12].TCoords == v2f{0.416666657f, 0.791666686f});
			CHECK(vertices[15].TCoords == v2f{0.208333328f, 0.583333313f});
		}
		{
			REQUIRE(sm.getMesh()->getMeshBuffer(2)->getVertexCount() == 24);
			auto vertices = static_cast<const irr::video::S3DVertex *>(
					sm.getMesh()->getMeshBuffer(2)->getVertices());
			CHECK(vertices[10].TCoords == v2f{0.375f, 0.416666657f});
			CHECK(vertices[11].TCoords == v2f{0.375f, 0.583333313f});
			CHECK(vertices[12].TCoords == v2f{0.708333313f, 0.625f});
			CHECK(vertices[17].TCoords == v2f{0.541666687f, 0.458333343f});
			CHECK(vertices[20].TCoords == v2f{0.208333328f, 0.416666657f});
			CHECK(vertices[22].TCoords == v2f{0.375f, 0.416666657f});
		}
	}
}
