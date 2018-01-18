#pragma once
#include <cassert>
#include "tile.h"

struct LayerRef
{
	const TileSpec *tile = nullptr;
	video::SColor color = video::SColor(0xFFFFFFFF);
	u8 material_flags = 0;
	u8 rotation = 0;
	u8 emissive_light = 0;
	u8 layer = 0;

	LayerRef() = default;

	LayerRef(const TileSpec *tile, int layer = 0) :
		tile(tile),
		layer(layer)
	{
	}

	const TileLayer &get() const
	{
		return tile->layers[layer];
	}

	explicit operator bool() const
	{
		return bool(get().texture_id);
	}

	explicit operator TileLayer() const
	{
		TileLayer result = get();
		result.color = color;
		result.material_flags = material_flags;
		return result;
	}

	const TileLayer *operator->() const
	{
		return &get();
	}

	bool operator==(const LayerRef &b) const
	{
		return get() == b.get();
	}
};

struct TileRef
{
	const TileSpec *tile = nullptr;
	video::SColor colors[MAX_TILE_LAYERS];
	u8 material_flags[MAX_TILE_LAYERS];
	u8 rotation = 0;
	u8 emissive_light = 0;

	TileRef() = default;

	TileRef(const TileSpec *tile) :
		tile(tile)
	{
		for (int k = 0; k < MAX_TILE_LAYERS; k++) {
			const TileLayer &layer = tile->layers[k];
			colors[k] = layer.color;
			material_flags[k] = layer.material_flags;
		}
	}

	TileRef(const TileSpec *tile, video::SColor color) :
		tile(tile)
	{
		for (int k = 0; k < MAX_TILE_LAYERS; k++) {
			const TileLayer &layer = tile->layers[k];
			colors[k] = layer.has_color ? layer.color : color;
			material_flags[k] = layer.material_flags;
		}
	}

	void setMaterialFlags(u8 set_flags, u8 clear_flags = 0)
	{
		u8 mask = ~clear_flags;
		for(u8 &mf : material_flags) {
			mf |= set_flags;
			mf &= mask;
		}
	}

	LayerRef getLayer(int layer = 0) const
	{
		assert((layer >= 0) && (layer < MAX_TILE_LAYERS));
		LayerRef ref;
		ref.tile = tile;
		ref.color = colors[layer];
		ref.material_flags = material_flags[layer];
		ref.rotation = rotation;
		ref.emissive_light = emissive_light;
		ref.layer = layer;
		return ref;
	}

	bool isTileable(const TileRef &b) const
	{
		return false; // FIXME: stub
	}

	const TileSpec *operator->() const
	{
		return tile;
	}

	LayerRef operator[](int layer) const
	{
		return getLayer(layer);
	}
};
