#include "extruder.h"

Extruder::Extruder(video::ITexture *_texture) :
	texture(_texture)
{
	if (texture->getColorFormat() != video::ECF_A8R8G8B8)
		return;
	const void *rawdata = texture->lock(video::ETLM_READ_ONLY);
	if (!rawdata)
		return;
	data = reinterpret_cast<const video::SColor *>(rawdata);
	auto size = texture->getSize();
	w = size.Width;
	h = size.Height;
	dw = 1.0f / w;
	dh = 1.0f / h;
	mesh.vertices.reserve(w * h);
	mesh.indices.reserve(w * h);
}

Extruder::~Extruder()
{
	if (data)
		texture->unlock();
}

inline video::SColor Extruder::pixel(u32 i, u32 j)
{
	return data[w * j + i];
}

inline bool Extruder::is_opaque(u32 i, u32 j)
{
	return pixel(i, j).getAlpha() >= threshold;
}

void Extruder::create_face(u32 i, u32 j, FaceDir dir)
{
	static const v3f normals[4] = {
		{0.0f, 1.0f, 0.0f},
		{0.0f, -1.0f, 0.0f},
		{-1.0f, 0.0f, 0.0f},
		{1.0f, 0.0f, 0.0f},
	};
	static const v2f tc[4] = {
		{0.5f, 0.5f},
		{0.5f, -0.5f},
		{0.5f, 0.5f},
		{-0.5f, 0.5f},
	};
	static const v3f vert[4][4] = {{ // Up (+Y)
		{0.0f, 0.0f, -0.5f},
		{0.0f, 0.0f, 0.5f},
		{1.0f, 0.0f, -0.5f},
		{1.0f, 0.0f, 0.5f},
	},{ // Down (-Y)
		{0.0f, 0.0f, -0.5f},
		{1.0f, 0.0f, -0.5f},
		{0.0f, 0.0f, 0.5f},
		{1.0f, 0.0f, 0.5f},
	},{ // Left (-X)
		{0.0f, 0.0f, -0.5f},
		{0.0f, -1.0f, -0.5f},
		{0.0f, 0.0f, 0.5f},
		{0.0f, -1.0f, 0.5f},
	},{ // Right (+X)
		{0.0f, 0.0f, -0.5f},
		{0.0f, 0.0f, 0.5f},
		{0.0f, -1.0f, -0.5f},
		{0.0f, -1.0f, 0.5f},
	}};
	static constexpr u16 ind[6] = {0, 1, 2, 3, 2, 1};
	float x = dw * i - 0.5f;
	float y = 0.5f - dh * j;
	u16 base = mesh.vertices.size();
	int m = static_cast<int>(dir);
	const v2f &t = tc[m];
	float tu = dw * (i + t.X);
	float tv = dh * (j + t.Y);
	const v3f &n = normals[m];
	for (int k = 0; k < 4; k++) {
		const v3f &v = vert[m][k];
		mesh.vertices.emplace_back(
			x + dw * v.X, y + dh * v.Y, thickness * v.Z,
			n.X, n.Y, n.Z,
			video::SColor(0xFFFFFFFF),
			tu, tv);
	}
	for (int k = 0; k < 6; k++)
		mesh.indices.push_back(base + ind[k]);
}

void Extruder::create_side(int id)
{
	static const v3f normals[2] = {
		{0.0f, 0.0f, -1.0f},
		{0.0f, 0.0f, 1.0f},
	};
	static const v2f vert[4] = {
		{0.0f, 1.0f},
		{0.0f, 0.0f},
		{1.0f, 1.0f},
		{1.0f, 0.0f},
	};
	static constexpr u16 ind[2][6] = {
		{0, 1, 2, 3, 2, 1},
		{0, 2, 1, 3, 1, 2},
	};
	static constexpr float zz[2] = {0.5f, -0.5f};
	u16 base = mesh.vertices.size();
	u16 base2 = mesh.overlay_vertices.size();
	float z = thickness * zz[id];
	const v3f &n = normals[id];
	for (int k = 0; k < 4; k++) {
		const v2f &v = vert[k];
		video::S3DVertex vert(
			v.X - 0.5f, 0.5f - v.Y, z,
			n.X, n.Y, n.Z,
			video::SColor(0xFFFFFFFF),
			v.X, v.Y);
		mesh.vertices.push_back(vert);
		mesh.overlay_vertices.push_back(vert);
	}
	for (int k = 0; k < 6; k++) {
		mesh.indices.push_back(base + ind[id][k]);
		mesh.overlay_indices.push_back(base2 + ind[id][k]);
	}
}

void Extruder::extrude()
{
	for (u32 j = 0; j < h; j++) {
		bool prev_opaque = false;
		for (u32 i = 0; i < w; i++) {
			bool opaque = is_opaque(i, j);
			if (opaque && !prev_opaque)
				create_face(i, j, FaceDir::Left);
			if (prev_opaque && !opaque)
				create_face(i, j, FaceDir::Right);
			prev_opaque = opaque;
		}
		if (prev_opaque)
			create_face(w, j, FaceDir::Right);
	}
	for (u32 i = 0; i < w; i++) {
		bool prev_opaque = false;
		for (u32 j = 0; j < h; j++) {
			bool opaque = is_opaque(i, j);
			if (opaque && !prev_opaque)
				create_face(i, j, FaceDir::Up);
			if (prev_opaque && !opaque)
				create_face(i, j, FaceDir::Down);
			prev_opaque = opaque;
		}
		if (prev_opaque)
			create_face(i, h, FaceDir::Down);
	}
	create_side(0);
	create_side(1);
}

ExtrudedMesh Extruder::takeMesh() noexcept
{
	return std::move(mesh);
}
