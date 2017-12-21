#pragma once
#include <vector>
#include "irrlichttypes_bloated.h"
#include <ITexture.h>
#include <SMeshBuffer.h>

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
	std::vector<video::S3DVertex> vertices;
	std::vector<u16> indices;

	video::SColor pixel(u32 i, u32 j);
	bool is_opaque(u32 i, u32 j);
	void create_face(u32 i, u32 j, FaceDir dir);
	void create_side(int id);

public:
	Extruder(video::ITexture *_texture);
	~Extruder();
	void extrude();
	scene::SMeshBuffer *createMesh();
};
