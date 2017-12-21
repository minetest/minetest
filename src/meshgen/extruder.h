#pragma once
#include <vector>
#include "irrlichttypes_bloated.h"
#include <ITexture.h>
#include <S3DVertex.h>

struct ExtrudedMesh
{
	std::vector<video::S3DVertex> vertices;
	std::vector<u16> indices;
	std::vector<video::S3DVertex> overlay_vertices;
	std::vector<u16> overlay_indices;

	explicit operator bool() const noexcept
	{
		return !vertices.empty();
	}
};

/*
 * Handles conversion of textures to extruded meshes.
 * Each instance may only be used once.
 */
class Extruder
{
	enum class FaceDir
	{
		Up,
		Down,
		Left,
		Right,
	};

	static constexpr int threshold = 128;
	static constexpr float thickness = 0.05f;
	video::ITexture *texture;
	const video::SColor *data;
	u32 w, h;
	float dw, dh;
	ExtrudedMesh mesh;

	video::SColor pixel(u32 i, u32 j);
	bool is_opaque(u32 i, u32 j);
	void create_face(u32 i, u32 j, FaceDir dir);
	void create_side(int id);

public:
	explicit Extruder(video::ITexture *_texture);
	Extruder(const Extruder &) = delete;
	Extruder(Extruder &&) = delete;
	~Extruder();
	Extruder &operator= (const Extruder &) = delete;
	Extruder &operator= (Extruder &&) = delete;
	void extrude();
	ExtrudedMesh takeMesh() noexcept;
};
