#include "CReadFile.h"
#include "vector3d.h"

#include <irrlicht.h>

// Catch needs to be included after Irrlicht so that it sees operator<<
// declarations.
#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <iostream>

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

TEST_CASE("load empty gltf file") {
	ScopedMesh sm("source/Irrlicht/tests/assets/empty.gltf");
	CHECK(sm.getMesh() == nullptr);
}

TEST_CASE("minimal triangle") {
	auto path = GENERATE(
			"source/Irrlicht/tests/assets/minimal_triangle.gltf",
			"source/Irrlicht/tests/assets/triangle_with_vertex_stride.gltf",
			// Test non-indexed geometry.
			"source/Irrlicht/tests/assets/triangle_without_indices.gltf");
	INFO(path);
	ScopedMesh sm(path);
	REQUIRE(sm.getMesh() != nullptr);
	REQUIRE(sm.getMesh()->getMeshBufferCount() == 1);

	SECTION("vertex coordinates are correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 3);
		const auto* vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(0)->getVertices());
		CHECK(vertices[0].Pos == irr::core::vector3df {0.0f, 0.0f, 0.0f});
		CHECK(vertices[1].Pos == irr::core::vector3df {1.0f, 0.0f, 0.0f});
		CHECK(vertices[2].Pos == irr::core::vector3df {0.0f, 1.0f, 0.0f});
	}

	SECTION("vertex indices are correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getIndexCount() == 3);
		const auto* indices = reinterpret_cast<irr::u16*>(
			sm.getMesh()->getMeshBuffer(0)->getIndices());
		CHECK(indices[0] == 2);
		CHECK(indices[1] == 1);
		CHECK(indices[2] == 0);
	}
}

TEST_CASE("blender cube") {
	ScopedMesh sm("source/Irrlicht/tests/assets/blender_cube.gltf");
	REQUIRE(sm.getMesh() != nullptr);
	REQUIRE(sm.getMesh()->getMeshBufferCount() == 1);
	SECTION("vertex coordinates are correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
		const auto* vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(0)->getVertices());
		CHECK(vertices[0].Pos == irr::core::vector3df{-10.0f, -10.0f, -10.0f});
		CHECK(vertices[3].Pos == irr::core::vector3df{-10.0f, 10.0f, -10.0f});
		CHECK(vertices[6].Pos == irr::core::vector3df{-10.0f, -10.0f, 10.0f});
		CHECK(vertices[9].Pos == irr::core::vector3df{-10.0f, 10.0f, 10.0f});
		CHECK(vertices[12].Pos == irr::core::vector3df{10.0f, -10.0f, -10.0f});
		CHECK(vertices[15].Pos == irr::core::vector3df{10.0f, 10.0f, -10.0f});
		CHECK(vertices[18].Pos == irr::core::vector3df{10.0f, -10.0f, 10.0f});
		CHECK(vertices[21].Pos == irr::core::vector3df{10.0f, 10.0f, 10.0f});
	}

	SECTION("vertex indices are correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getIndexCount() == 36);
		const auto* indices = reinterpret_cast<irr::u16*>(
			sm.getMesh()->getMeshBuffer(0)->getIndices());
		CHECK(indices[0] == 16);
		CHECK(indices[1] == 5);
		CHECK(indices[2] == 22);
		CHECK(indices[35] == 0);
	}

	SECTION("vertex normals are correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
		const auto* vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(0)->getVertices());
		CHECK(vertices[0].Normal == irr::core::vector3df{-1.0f, 0.0f, 0.0f});
		CHECK(vertices[1].Normal == irr::core::vector3df{0.0f, -1.0f, 0.0f});
		CHECK(vertices[2].Normal == irr::core::vector3df{0.0f, 0.0f, -1.0f});
		CHECK(vertices[3].Normal == irr::core::vector3df{-1.0f, 0.0f, 0.0f});
		CHECK(vertices[6].Normal == irr::core::vector3df{-1.0f, 0.0f, 0.0f});
		CHECK(vertices[23].Normal == irr::core::vector3df{1.0f, 0.0f, 0.0f});

	}

	SECTION("texture coords are correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
		const auto* vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(0)->getVertices());
		CHECK(vertices[0].TCoords == irr::core::vector2df{0.375f, 1.0f});
		CHECK(vertices[1].TCoords == irr::core::vector2df{0.125f, 0.25f});
		CHECK(vertices[2].TCoords == irr::core::vector2df{0.375f, 0.0f});
		CHECK(vertices[3].TCoords == irr::core::vector2df{0.6250f, 1.0f});
		CHECK(vertices[6].TCoords == irr::core::vector2df{0.375f, 0.75f});
	}
}

TEST_CASE("mesh loader returns nullptr when given null file pointer") {
	ScopedMesh sm(nullptr);
	CHECK(sm.getMesh() == nullptr);
}

TEST_CASE("invalid JSON returns nullptr") {
	ScopedMesh sm("source/Irrlicht/tests/assets/json_missing_brace.gltf");
	CHECK(sm.getMesh() == nullptr);
}

TEST_CASE("blender cube scaled") {
	ScopedMesh sm("source/Irrlicht/tests/assets/blender_cube_scaled.gltf");
	REQUIRE(sm.getMesh() != nullptr);
	REQUIRE(sm.getMesh()->getMeshBufferCount() == 1);
	
	SECTION("Scaling is correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
		const auto* vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(0)->getVertices());

		CHECK(vertices[0].Pos == irr::core::vector3df{-150.0f, -1.0f, -21.5f});
		CHECK(vertices[3].Pos == irr::core::vector3df{-150.0f, 1.0f, -21.5f});
		CHECK(vertices[6].Pos == irr::core::vector3df{-150.0f, -1.0f, 21.5f});
		CHECK(vertices[9].Pos == irr::core::vector3df{-150.0f, 1.0f, 21.5f});
		CHECK(vertices[12].Pos == irr::core::vector3df{150.0f, -1.0f, -21.5f});
		CHECK(vertices[15].Pos == irr::core::vector3df{150.0f, 1.0f, -21.5f});
		CHECK(vertices[18].Pos == irr::core::vector3df{150.0f, -1.0f, 21.5f});
		CHECK(vertices[21].Pos == irr::core::vector3df{150.0f, 1.0f, 21.5f});
	}
}

TEST_CASE("blender cube matrix transform") {
	ScopedMesh sm("source/Irrlicht/tests/assets/blender_cube_matrix_transform.gltf");
	REQUIRE(sm.getMesh() != nullptr);
	REQUIRE(sm.getMesh()->getMeshBufferCount() == 1);
	
	SECTION("Transformation is correct") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
		const auto* vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(0)->getVertices());
		const auto checkVertex = [&](const std::size_t i, irr::core::vector3df vec) {
			// The transform scales by (1, 2, 3) and translates by (4, 5, 6).
			CHECK(vertices[i].Pos == vec * irr::core::vector3df{1, 2, 3}
					// The -6 is due to the coordinate system conversion.
					+ irr::core::vector3df{4, 5, -6});
		};
		checkVertex(0, irr::core::vector3df{-1, -1, -1});
		checkVertex(3, irr::core::vector3df{-1, 1, -1});
		checkVertex(6, irr::core::vector3df{-1, -1, 1});
		checkVertex(9, irr::core::vector3df{-1, 1, 1});
		checkVertex(12, irr::core::vector3df{1, -1, -1});
		checkVertex(15, irr::core::vector3df{1, 1, -1});
		checkVertex(18, irr::core::vector3df{1, -1, 1});
		checkVertex(21, irr::core::vector3df{1, 1, 1});
	}
}

TEST_CASE("snow man") {
	ScopedMesh sm("source/Irrlicht/tests/assets/snow_man.gltf");
	REQUIRE(sm.getMesh() != nullptr);
	REQUIRE(sm.getMesh()->getMeshBufferCount() == 3);

	SECTION("vertex coordinates are correct for all buffers") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
		const auto* vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(0)->getVertices());
		
		CHECK(vertices[0].Pos == irr::core::vector3df{3.0f, 24.0f, -3.0f});
		CHECK(vertices[3].Pos == irr::core::vector3df{3.0f, 18.0f, 3.0f});
		CHECK(vertices[6].Pos == irr::core::vector3df{-3.0f, 18.0f, -3.0f});
		CHECK(vertices[9].Pos == irr::core::vector3df{3.0f, 24.0f, 3.0f});
		CHECK(vertices[12].Pos == irr::core::vector3df{3.0f, 18.0f, -3.0f});
		CHECK(vertices[15].Pos == irr::core::vector3df{-3.0f, 18.0f, 3.0f});
		CHECK(vertices[18].Pos == irr::core::vector3df{3.0f, 18.0f, -3.0f});
		CHECK(vertices[21].Pos == irr::core::vector3df{3.0f, 18.0f, 3.0f});
		
		vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(1)->getVertices());
		
		CHECK(vertices[2].Pos == irr::core::vector3df{5.0f, 10.0f, 5.0f});
		CHECK(vertices[3].Pos == irr::core::vector3df{5.0f, 0.0f, 5.0f});
		CHECK(vertices[7].Pos == irr::core::vector3df{-5.0f, 0.0f, 5.0f});
		CHECK(vertices[8].Pos == irr::core::vector3df{5.0f, 10.0f, -5.0f});
		CHECK(vertices[14].Pos == irr::core::vector3df{5.0f, 0.0f, 5.0f});
		CHECK(vertices[16].Pos == irr::core::vector3df{5.0f, 10.0f, -5.0f});
		CHECK(vertices[22].Pos == irr::core::vector3df{-5.0f, 10.0f, 5.0f});
		CHECK(vertices[23].Pos == irr::core::vector3df{-5.0f, 0.0f, 5.0f});

		vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(2)->getVertices());
		
		CHECK(vertices[1].Pos == irr::core::vector3df{4.0f, 10.0f, -4.0f});
		CHECK(vertices[2].Pos == irr::core::vector3df{4.0f, 18.0f, 4.0f});
		CHECK(vertices[3].Pos == irr::core::vector3df{4.0f, 10.0f, 4.0f});
		CHECK(vertices[10].Pos == irr::core::vector3df{-4.0f, 18.0f, -4.0f});
		CHECK(vertices[11].Pos == irr::core::vector3df{-4.0f, 18.0f, 4.0f});
		CHECK(vertices[12].Pos == irr::core::vector3df{4.0f, 10.0f, -4.0f});
		CHECK(vertices[17].Pos == irr::core::vector3df{-4.0f, 18.0f, -4.0f});
		CHECK(vertices[18].Pos == irr::core::vector3df{4.0f, 10.0f, -4.0f});
	}

	SECTION("vertex indices are correct for all buffers") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getIndexCount() == 36);
		const auto* indices = reinterpret_cast<irr::u16*>(
			sm.getMesh()->getMeshBuffer(0)->getIndices());
		CHECK(indices[0] == 23);
		CHECK(indices[1] == 21);
		CHECK(indices[2] == 22);
		CHECK(indices[35] == 2);

		REQUIRE(sm.getMesh()->getMeshBuffer(1)->getIndexCount() == 36);
		indices = reinterpret_cast<irr::u16*>(
			sm.getMesh()->getMeshBuffer(1)->getIndices());
		CHECK(indices[10] == 16);
		CHECK(indices[11] == 18);
		CHECK(indices[15] == 13);
		CHECK(indices[27] == 5);

		REQUIRE(sm.getMesh()->getMeshBuffer(1)->getIndexCount() == 36);
		indices = reinterpret_cast<irr::u16*>(
			sm.getMesh()->getMeshBuffer(2)->getIndices());
		CHECK(indices[26] == 6);
		CHECK(indices[27] == 5);
		CHECK(indices[29] == 6);
		CHECK(indices[32] == 2);
	}

	
	SECTION("vertex normals are correct for all buffers") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
		const auto* vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(0)->getVertices());
		CHECK(vertices[0].Normal == irr::core::vector3df{1.0f, 0.0f, -0.0f});
		CHECK(vertices[1].Normal == irr::core::vector3df{1.0f, 0.0f, -0.0f});
		CHECK(vertices[2].Normal == irr::core::vector3df{1.0f, 0.0f, -0.0f});
		CHECK(vertices[3].Normal == irr::core::vector3df{1.0f, 0.0f, -0.0f});
		CHECK(vertices[6].Normal == irr::core::vector3df{-1.0f, 0.0f, -0.0f});
		CHECK(vertices[23].Normal == irr::core::vector3df{0.0f, 0.0f, 1.0f});

		vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(1)->getVertices());
		
		CHECK(vertices[0].Normal == irr::core::vector3df{1.0f, 0.0f, -0.0f});
		CHECK(vertices[1].Normal == irr::core::vector3df{1.0f, 0.0f, -0.0f});
		CHECK(vertices[3].Normal == irr::core::vector3df{1.0f, 0.0f, -0.0f});
		CHECK(vertices[6].Normal == irr::core::vector3df{-1.0f, 0.0f, -0.0f});
		CHECK(vertices[7].Normal == irr::core::vector3df{-1.0f, 0.0f, -0.0f});
		CHECK(vertices[22].Normal == irr::core::vector3df{0.0f, 0.0f, 1.0f});


		vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(2)->getVertices());

		CHECK(vertices[3].Normal == irr::core::vector3df{1.0f, 0.0f, -0.0f});
		CHECK(vertices[4].Normal == irr::core::vector3df{-1.0f, 0.0f, -0.0f});
		CHECK(vertices[5].Normal == irr::core::vector3df{-1.0f, 0.0f, -0.0f});
		CHECK(vertices[10].Normal == irr::core::vector3df{0.0f, 1.0f, -0.0f});
		CHECK(vertices[11].Normal == irr::core::vector3df{0.0f, 1.0f, -0.0f});
		CHECK(vertices[19].Normal == irr::core::vector3df{0.0f, 0.0f, -1.0f});

	}

	
	SECTION("texture coords are correct for all buffers") {
		REQUIRE(sm.getMesh()->getMeshBuffer(0)->getVertexCount() == 24);
		const auto* vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(0)->getVertices());

		CHECK(vertices[0].TCoords == irr::core::vector2df{0.583333, 0.791667});
		CHECK(vertices[1].TCoords == irr::core::vector2df{0.583333, 0.666667});
		CHECK(vertices[2].TCoords == irr::core::vector2df{0.708333, 0.791667});
		CHECK(vertices[5].TCoords == irr::core::vector2df{0.375, 0.416667});
		CHECK(vertices[6].TCoords == irr::core::vector2df{0.5, 0.291667});
		CHECK(vertices[19].TCoords == irr::core::vector2df{0.708333, 0.75});

		vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(1)->getVertices());

		CHECK(vertices[1].TCoords == irr::core::vector2df{0, 0.791667});
		CHECK(vertices[4].TCoords == irr::core::vector2df{0.208333, 0.791667});
		CHECK(vertices[5].TCoords == irr::core::vector2df{0, 0.791667});
		CHECK(vertices[6].TCoords == irr::core::vector2df{0.208333, 0.583333});
		CHECK(vertices[12].TCoords == irr::core::vector2df{0.416667, 0.791667});
		CHECK(vertices[15].TCoords == irr::core::vector2df{0.208333, 0.583333});

		vertices = reinterpret_cast<irr::video::S3DVertex*>(
			sm.getMesh()->getMeshBuffer(2)->getVertices());

		CHECK(vertices[10].TCoords == irr::core::vector2df{0.375, 0.416667});
		CHECK(vertices[11].TCoords == irr::core::vector2df{0.375, 0.583333});
		CHECK(vertices[12].TCoords == irr::core::vector2df{0.708333, 0.625});
		CHECK(vertices[17].TCoords == irr::core::vector2df{0.541667, 0.458333});
		CHECK(vertices[20].TCoords == irr::core::vector2df{0.208333, 0.416667});
		CHECK(vertices[22].TCoords == irr::core::vector2df{0.375, 0.416667});
	}
}
