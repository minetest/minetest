#pragma once

#include <functional>
#include <array>
#include <memory>
#include <variant>
#include <string>
#include <vector>
#include <utility>

#include "constants.h"
#include "irrlichttypes.h"
#include "nameidmapping.h"
#include "nodedef.h"

class IdIdMapping;

#define MODIFIER_IGNORE ((u16)-1)


enum ModifiedProperty : u64 {
	MP_LightSource = u64{1} << 0,
};

struct NodeModifier {
	NodeModifier(const std::string &name, bool is_dummy=false)
			: m_name(name), m_is_dummy(is_dummy) {};

	std::string m_name;
	bool m_is_dummy;

	u8 m_light_source = 0;

	u64 m_modified_props_mask = 0;

	void serialize(std::ostream &os, u16 protocol_version) const;
	void deSerialize(std::istream &is, u16 protocol_version);
};

class NodeModifierManager {
public:
	NodeModifierManager() {};

	u16 add(NodeModifier &&nodeModifier);
	u16 getId(const std::string &name) const;
	const NodeModifier& get(u16 id) const;

	void serialize(std::ostream &os, u16 protocol_version) const;
	void deSerialize(std::istream &is, u16 protocol_version);

	inline u16 size() const {
		return m_modifiers.size();
	}


private:
	NameIdMapping m_name_id_mapping;
	std::vector<NodeModifier> m_modifiers;
	const inline static NodeModifier dummy{"<unknown>", true};
};

NodeModifierManager *createNodeModifierManager();

// Resolved overrides only; properties absent from mask retain the base definition.
struct NodePropertyOverrides {
	u64 mask = 0;

	u8 light_source = 0;

	ContentLightingFlags apply(ContentLightingFlags base) const;
};

struct AppliedNodeModifier {
	v3s16 pos;
	u16 id;
};

class AppliedNodeModifiersList {
	class Leaf {
	public:
		using ConstIterator = std::vector<AppliedNodeModifier>::const_iterator;
		using ConstRange = std::pair<ConstIterator, ConstIterator>;

		Leaf() = default;
		explicit Leaf(std::vector<AppliedNodeModifier> entries) :
				m_entries(std::move(entries)) {}
		Leaf(ConstIterator first, ConstIterator last) : m_entries(first, last) {}

		size_t size() const { return m_entries.size(); }
		ConstIterator begin() const { return m_entries.begin(); }
		ConstIterator end() const { return m_entries.end(); }
		ConstRange findRange(v3s16 pos) const;
		bool add(v3s16 pos, u16 id, const NodeModifierManager *prune_manager);
		bool set(v3s16 pos, const std::vector<u16> &ids);
		bool remove(v3s16 pos, u16 id);
		bool remove(v3s16 pos);
		void remapIds(const std::function<u16(u16)> &map_id);

	private:
		std::vector<AppliedNodeModifier> m_entries;

		bool addWithPrune(v3s16 pos, u16 id, const NodeModifierManager *prune_manager);
	};

	struct Bucket {
		using TreeNode = std::unique_ptr<std::array<Bucket, MAP_BLOCKSIZE>>;

		std::variant<Leaf, TreeNode> data;

		u8 coordinate(v3s16 pos, u8 depth) const {
			u8 coord = depth == 0 ? pos.X : depth == 1 ? pos.Y : pos.Z;
			assert(depth < 3 && coord >= 0 && coord < MAP_BLOCKSIZE);
			return coord;
		}

		Bucket *try_get_children() {
			auto *tree = std::get_if<TreeNode>(&data);
			return tree ? (*tree)->data() : nullptr;
		}
		const Bucket *try_get_children() const {
			const auto *tree = std::get_if<TreeNode>(&data);
			return tree ? (*tree)->data() : nullptr;
		}
		Leaf &get_leaf() { return std::get<Leaf>(data); }
		const Leaf &get_leaf() const { return std::get<Leaf>(data); }

		Bucket() = default;
		Bucket(Bucket &&) noexcept = default;
		Bucket &operator=(Bucket &&) noexcept = default;
		// Split only overflowing leaves; deletion never coalesces branches.
		void split(u8 depth);
		// Partition the source range before allocating storage for final leaves.
		void assignRange(Leaf::ConstIterator first, Leaf::ConstIterator last, u8 depth);
		const Bucket &findLeaf(v3s16 pos, u8 &depth) const;
		// Route edits to one leaf and update the owning list's entry count.
		bool add(v3s16 pos, u16 id, size_t &total_size,
				const NodeModifierManager *prune_manager, u8 depth = 0);
		bool set(v3s16 pos, const std::vector<u16> &ids, size_t &total_size, u8 depth = 0);
		bool remove(v3s16 pos, u16 id, size_t &total_size, u8 depth = 0);
		bool remove(v3s16 pos, size_t &total_size, u8 depth = 0);

		template <typename F>
		void forEachLeaf(F &&callback) const {
			if (const Bucket *branch = try_get_children()) {
				for (size_t i = 0; i < MAP_BLOCKSIZE; ++i)
					branch[i].forEachLeaf(callback);
			} else {
				callback(get_leaf());
			}
		}
		template <typename F>
		void forEachLeaf(F &&callback) {
			if (auto *branch = try_get_children()) {
				for (size_t i = 0; i < MAP_BLOCKSIZE; ++i)
					branch[i].forEachLeaf(callback);
			} else {
				callback(get_leaf());
			}
		}
	};

public:
	AppliedNodeModifiersList() = default;
	AppliedNodeModifiersList(const AppliedNodeModifiersList &) = delete;
	void clear() noexcept {
		m_root.data.emplace<Leaf>();
		m_size = 0;
	}

	using ConstIterator = Leaf::ConstIterator;
	using ConstRange = std::pair<ConstIterator, ConstIterator>;
	ConstRange findRange(v3s16 pos) const;

	// At most three coordinate lookups, then search one sorted leaf.
	NodePropertyOverrides resolve(v3s16 pos, const NodeModifierManager &manager) const;

	class Cursor {
	public:
		// Positions must be nondecreasing in X/Y/Z order. Repeated positions
		// reuse the resolved value; leaves are traversed without allocation.
		NodePropertyOverrides resolve(v3s16 pos);

	private:
		friend class AppliedNodeModifiersList;
		Cursor(const Bucket &root, const NodeModifierManager &manager, bool allow_skipping);
		void descend();
		void nextLeaf();

		std::array<const Bucket *, 4> m_path{};
		std::array<u8, 3> m_child{};
		u8 m_depth = 0;
		bool m_finished = false;
		ConstIterator m_next, m_end;
		const NodeModifierManager &m_manager;
		bool m_allow_skipping;
		bool m_has_position = false;
		v3s16 m_position;
		NodePropertyOverrides m_value;
	};

	// The list and manager must outlive the cursor and remain unchanged.
	// All list mutations invalidate ranges and cursors.
	Cursor cursor(const NodeModifierManager &manager, bool allow_skipping = false) const {
		return Cursor(m_root, manager, allow_skipping);
	}

	size_t size() const { return m_size; }
	bool empty() const { return m_size == 0; }

	// Visit entries in X/Y/Z order, preserving modifier order within each position.
	// The callback must not mutate this list.
	template <typename F>
	void forEach(F &&callback) const {
		m_root.forEachLeaf([&](const Leaf &leaf) {
			for (const auto &entry : leaf)
				callback(entry);
		});
	}

	// Only block-local coordinates (0..15 on each axis) can be stored.
	// Supplying a manager opts into removing fully overridden modifiers.
	bool add(v3s16 pos, u16 id, const NodeModifierManager *prune_manager = nullptr);
	// Replace one node's modifiers, keeping the last occurrence of duplicate IDs.
	// Invalid positions/IDs or insufficient capacity leave the list unchanged.
	bool set(v3s16 pos, const std::vector<u16> &ids);
	bool remove(v3s16 pos, u16 id);
	bool remove(v3s16 pos);

	// Disk entries must already use the local IDs described by nimap.
	void serialize(std::ostream &os, u16 version, bool disk, const NameIdMapping &nimap,
			const IdIdMapping *id_mapping = nullptr) const;
	// Disk entries retain local IDs; nimap receives their names.
	void deSerialize(std::istream &is, u16 version, bool disk, NameIdMapping &nimap);
	void remapIds(const std::function<u16(u16)> &map_id);

private:
	// Bulk construction splits at N; live updates at 2N. Mutation benchmarks
	// favor 32-64 over larger thresholds; keep 64 to delay branch allocation.
	static constexpr size_t LEAF_TARGET = 64;
	Bucket m_root;
	size_t m_size = 0;
};

// Flat, independently owned data for mesh workers. Positions are relative to the
// mesh origin and may lie outside a single block. Build by appending block lists;
// appending invalidates ranges and cursors obtained from this snapshot.
class AppliedNodeModifiersSnapshot {
public:
	using ConstIterator = std::vector<AppliedNodeModifier>::const_iterator;
	using ConstRange = std::pair<ConstIterator, ConstIterator>;

	class Cursor {
	public:
		// Positions must be nondecreasing in v3s16's X/Y/Z ordering.
		// Repeated positions reuse the resolved value. A scan costs O(N + M).
		NodePropertyOverrides resolve(v3s16 pos);

	private:
		friend class AppliedNodeModifiersSnapshot;
		Cursor(ConstIterator first, ConstIterator last, const NodeModifierManager &manager,
				bool allow_skipping) :
				m_next(first), m_end(last), m_manager(manager),
				m_allow_skipping(allow_skipping) {}

		ConstIterator m_next;
		ConstIterator m_end;
		const NodeModifierManager &m_manager;
		bool m_allow_skipping;
		bool m_has_position = false;
		v3s16 m_position;
		NodePropertyOverrides m_value;
	};

	void append(const AppliedNodeModifiersList &other, v3s16 offset);
	size_t size() const { return m_list.size(); }
	bool empty() const { return m_list.empty(); }
	void shrink_to_fit() { m_list.shrink_to_fit(); }
	ConstRange findRange(v3s16 pos) const;
	NodePropertyOverrides resolve(v3s16 pos, const NodeModifierManager &manager) const;
	// The snapshot and manager must outlive the cursor and remain unchanged.
	Cursor cursor(const NodeModifierManager &manager, bool allow_skipping = false) const {
		return Cursor(m_list.begin(), m_list.end(), manager, allow_skipping);
	}

private:
	std::vector<AppliedNodeModifier> m_list;
};
