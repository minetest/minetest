#include "node_modifier.h"
#include "constants.h"
#include "ididmapping.h"

#include "util/numeric.h"
#include "util/serialize.h"
#include <algorithm>
#include <cassert>

static_assert(MAP_BLOCKSIZE == 16, "Packed modifier positions require 16-node blocks");

void NodeModifier::serialize(std::ostream &os, u16 protocol_version) const {
	writeU64(os, m_modified_props_mask);
	writeS8(os, m_is_dummy);
	if (m_modified_props_mask & MP_LightSource) {
		writeU8(os, m_light_source);
	}
}

void NodeModifier::deSerialize(std::istream &is, u16 protocol_version) {
	m_modified_props_mask = readU64(is);
	m_is_dummy = readS8(is);

	if (m_modified_props_mask & MP_LightSource) {
		m_light_source = readU8(is);
	}
}

u16 NodeModifierManager::add(NodeModifier &&nodeModifier) {
	u16 existing_id;
	if (m_name_id_mapping.getId(nodeModifier.m_name, existing_id)) {
		// Replace if a modifier with the same name already exists
		m_modifiers[existing_id] = std::move(nodeModifier);

		return existing_id;
	}

	if (m_modifiers.size() >= MODIFIER_IGNORE)
		return MODIFIER_IGNORE;

	m_name_id_mapping.set(m_modifiers.size(), nodeModifier.m_name);
	m_modifiers.push_back(std::move(nodeModifier));

	return m_modifiers.size() - 1;
}

u16 NodeModifierManager::getId(const std::string &name) const {
	u16 id;
	bool is_present = m_name_id_mapping.getId(name, id);
	if (is_present)
		return id;
	else
		return MODIFIER_IGNORE;
}

const NodeModifier& NodeModifierManager::get(u16 id) const {
	if (id >= m_modifiers.size())
		return dummy;

	return m_modifiers[id];
}

void NodeModifierManager::serialize(std::ostream &os, u16 protocol_version) const {
	u16 size = m_modifiers.size();
	writeU16(os, size);

	for (u16 i = 0; i < size; i++) {
		const NodeModifier& cur = m_modifiers[i];
		os << serializeString16(cur.m_name);
		cur.serialize(os, protocol_version);
	}
}

void NodeModifierManager::deSerialize(std::istream &is, u16 protocol_version) {
	u16 modifiers_num = readU16(is);

	m_modifiers.clear();
	m_name_id_mapping.clear();
	m_modifiers.reserve(modifiers_num);

	for (u16 i = 0; i < modifiers_num; i++) {
		std::string name = deSerializeString16(is);
		if (name.empty()) {
			throw SerializationError("NodeModifierManager::deSerialize(): "
				"The name of the node modifier is empty");
		}

		u16 existing_id;
		if (m_name_id_mapping.getId(name, existing_id)) {
			throw SerializationError("NodeModifierManager::deSerialize(): "
				"Node modifier with the name " + name + " was already deserialized");
		}

		m_name_id_mapping.set(m_modifiers.size(), name);
		m_modifiers.emplace_back(name);
		m_modifiers[i].deSerialize(is, protocol_version);
	}
}

NodeModifierManager *createNodeModifierManager() {
	return new NodeModifierManager();
}

ContentLightingFlags NodePropertyOverrides::apply(ContentLightingFlags base) const
{
	if (mask & MP_LightSource)
		base.light_source = light_source;
	return base;
}

static NodePropertyOverrides resolveInRange(
		AppliedNodeModifiersList::ConstRange range, const NodeModifierManager &manager)
{
	NodePropertyOverrides result;
	for (auto it = range.first; it != range.second; ++it) {
		const NodeModifier &modifier = manager.get(it->id);
		if (modifier.m_is_dummy)
			continue;

		if (modifier.m_modified_props_mask & MP_LightSource) {
			result.mask |= MP_LightSource;
			result.light_source = modifier.m_light_source;
		}
	}
	return result;
}

NodePropertyOverrides AppliedNodeModifiersList::resolve(v3s16 pos,
		const NodeModifierManager &manager) const
{
	return resolveInRange(findRange(pos), manager);
}

NodePropertyOverrides AppliedNodeModifiersSnapshot::Cursor::resolve(v3s16 pos)
{
	assert(!m_has_position || !(pos < m_position));

	if (m_has_position && pos == m_position)
		return m_value;

	if (m_allow_skipping) {
		while (m_next != m_end && m_next->pos < pos)
			++m_next;
	} else {
		assert(m_next == m_end || m_next->pos >= pos);
	}

	ConstIterator first = m_next;

	while (m_next != m_end && m_next->pos == pos)
		++m_next;

	m_value = resolveInRange({first, m_next}, m_manager);
	m_position = pos;
	m_has_position = true;

	return m_value;
}

// Shared by leaves and snapshots.
template <typename Entries>
static auto findModifierRange(Entries &entries, v3s16 pos)
{
	auto first = std::lower_bound(entries.begin(), entries.end(), pos,
			[](const AppliedNodeModifier &entry, v3s16 pos) { return entry.pos < pos; });

	auto last = first;
	while (last != entries.end() && last->pos == pos)
		++last;

	return std::make_pair(first, last);
}

void AppliedNodeModifiersList::Bucket::split(u8 depth)
{
	auto *leaf = std::get_if<Leaf>(&data);

	assignRange(leaf->begin(), leaf->end(), depth);
}

void AppliedNodeModifiersList::Bucket::assignRange(
		Leaf::ConstIterator first, Leaf::ConstIterator last, u8 depth)
{
	if (static_cast<size_t>(last - first) <= LEAF_TARGET || depth == 3) {
		data.emplace<Leaf>(first, last);
		return;
	}

	auto children = std::make_unique<std::array<Bucket, MAP_BLOCKSIZE>>();

	while (first != last) {
		const u8 index = coordinate(first->pos, depth);

		// Earlier coordinates are fixed within this bucket, so the current
		// coordinate is sorted and its boundary can be found by binary search.
		auto end = std::upper_bound(first + 1, last, index,
				[this, depth](u8 value, const AppliedNodeModifier &entry) {
					return value < coordinate(entry.pos, depth);
				});

		(*children)[index].assignRange(first, end, depth + 1);

		first = end;
	}

	// Keep the source leaf alive until all of its ranges have been copied.
	data = std::move(children);
}

const AppliedNodeModifiersList::Bucket &AppliedNodeModifiersList::Bucket::findLeaf(
		v3s16 pos, u8 &depth) const
{
	const Bucket *bucket = this;
	while (const auto *children = bucket->try_get_children())
		bucket = &children[coordinate(pos, depth++)];

	return *bucket;
}

AppliedNodeModifiersList::Cursor::Cursor(const Bucket &root,
		const NodeModifierManager &manager, bool allow_skipping) :
		m_manager(manager), m_allow_skipping(allow_skipping)
{
	m_path[0] = &root;

	descend();

	if (m_next == m_end)
		nextLeaf();
}

void AppliedNodeModifiersList::Cursor::descend()
{
	while (const auto *children = m_path[m_depth]->try_get_children()) {
		assert(m_depth < 3);
		m_child[m_depth] = 0;
		m_path[++m_depth] = children;
	}

	const auto &leaf = m_path[m_depth]->get_leaf();
	m_next = leaf.begin();
	m_end = leaf.end();
}

void AppliedNodeModifiersList::Cursor::nextLeaf()
{
	while (m_depth) {
		const u8 parent = m_depth - 1;
		if (++m_child[parent] < MAP_BLOCKSIZE) {
			m_path[m_depth] = &m_path[parent]->try_get_children()[m_child[parent]];

			descend();

			if (m_next != m_end)
				return;
		} else {
			--m_depth;
		}
	}

	m_finished = true;
}

NodePropertyOverrides AppliedNodeModifiersList::Cursor::resolve(v3s16 pos)
{
	assert(!m_has_position || pos >= m_position);

	if (m_has_position && pos == m_position)
		return m_value;

	if (m_allow_skipping) {
		while (!m_finished && m_next->pos < pos) {
			if (++m_next == m_end)
				nextLeaf();
		}
	} else {
		assert(m_finished || m_next->pos >= pos);
	}

	m_value = {};
	if (!m_finished && m_next->pos == pos) {
		auto first = m_next;
		while (m_next != m_end && m_next->pos == pos)
			++m_next;

		m_value = resolveInRange({first, m_next}, m_manager);
		if (m_next == m_end)
			nextLeaf();
	}

	m_position = pos;
	m_has_position = true;

	return m_value;
}

AppliedNodeModifiersList::Leaf::ConstRange AppliedNodeModifiersList::Leaf::findRange(v3s16 pos) const
{
	return findModifierRange(m_entries, pos);
}

void AppliedNodeModifiersList::Leaf::remapIds(const std::function<u16(u16)> &map_id)
{
	for (auto &entry : m_entries)
		entry.id = map_id(entry.id);
}

bool AppliedNodeModifiersList::Leaf::addWithPrune(v3s16 pos, u16 id,
		const NodeModifierManager *prune_manager)
{
	// Keep survivors in reverse order until capacity has been checked.
	std::array<u16, MAX_NODE_MODIFIERS + 1> ids;
	size_t count = 1;
	ids[0] = id;

	auto [first, last] = findModifierRange(m_entries, pos);

	const auto &added = prune_manager->get(id);
	u64 covered = added.m_is_dummy ? 0 : added.m_modified_props_mask;

	for (auto it = last; it != first;) {
		const u16 current = (--it)->id;
		if (current == id)
			continue;

		const auto &modifier = prune_manager->get(current);
		const u64 mask = modifier.m_modified_props_mask;
		if (mask == 0 || modifier.m_is_dummy || (covered | mask) != covered) {
			covered |= mask;
			assert(count <= MAX_NODE_MODIFIERS);
			ids[count++] = current;
		}
	}

	if (count > MAX_NODE_MODIFIERS)
		return false;

	const size_t offset = first - m_entries.begin();
	const size_t old_count = last - first;

	if (count > old_count)
		m_entries.insert(last, {pos, id});
	else {
		if (count < old_count)
			m_entries.erase(first + count, last);

		for (size_t i = 0; i < count; ++i)
			m_entries[offset + i] = {pos, ids[count - 1 - i]};
	}

	return true;
}

bool AppliedNodeModifiersList::Leaf::add(v3s16 pos, u16 id,
		const NodeModifierManager *prune_manager)
{
	if (id == MODIFIER_IGNORE)
		return false;

	if (prune_manager) {
		return addWithPrune(pos, id, prune_manager);
	}

	auto [first, last] = findModifierRange(m_entries, pos);
	for (auto it = first; it != last; ++it) {
		if (it->id == id) {
			std::rotate(it, it + 1, last);
			return true;
		}
	}

	if (static_cast<size_t>(last - first) >= MAX_NODE_MODIFIERS)
		return false;

	m_entries.insert(last, {pos, id});

	return true;
}

bool AppliedNodeModifiersList::Leaf::set(v3s16 pos, const std::vector<u16> &ids)
{
	std::array<u16, MAX_NODE_MODIFIERS> unique;
	size_t count = 0;
	for (auto it = ids.rbegin(); it != ids.rend(); ++it) {
		if (*it == MODIFIER_IGNORE)
			return false;

		const auto end = unique.begin() + count;
		if (std::find(unique.begin(), end, *it) != end)
			continue;

		if (count == unique.size())
			return false;

		unique[count++] = *it;
	}

	auto [first, last] = findModifierRange(m_entries, pos);
	const size_t offset = first - m_entries.begin();
	const size_t old_count = last - first;

	if (count > old_count)
		m_entries.insert(last, count - old_count, AppliedNodeModifier{pos, 0});
	else if (count < old_count)
		m_entries.erase(first + count, last);
	for (size_t i = 0; i < count; ++i)
		m_entries[offset + i] = {pos, unique[count - 1 - i]};

	return true;
}

bool AppliedNodeModifiersList::Leaf::remove(v3s16 pos, u16 id)
{
	auto it = std::lower_bound(m_entries.begin(), m_entries.end(), pos,
			[](const AppliedNodeModifier &entry, v3s16 p) { return entry.pos < p; });
	for (; it != m_entries.end() && it->pos == pos; ++it) {
		if (it->id == id) {
			m_entries.erase(it);
			return true;
		}
	}

	return false;
}

bool AppliedNodeModifiersList::Leaf::remove(v3s16 pos)
{
	auto [first, last] = findModifierRange(m_entries, pos);
	if (first == last)
		return false;

	m_entries.erase(first, last);

	return true;
}

bool AppliedNodeModifiersList::Bucket::add(v3s16 pos, u16 id, size_t &total_size,
		const NodeModifierManager *prune_manager, u8 depth)
{
	if (auto *branch = try_get_children())
		return branch[coordinate(pos, depth)].add(pos, id, total_size, prune_manager, depth + 1);

	auto &entries = get_leaf();
	const size_t old_size = entries.size();
	if (!entries.add(pos, id, prune_manager))
		return false;

	total_size = total_size - old_size + entries.size();
	if (entries.size() > LEAF_TARGET && depth < 3)
		split(depth);

	return true;
}

bool AppliedNodeModifiersList::Bucket::set(v3s16 pos, const std::vector<u16> &ids,
		size_t &total_size, u8 depth)
{
	if (auto *branch = try_get_children())
		return branch[coordinate(pos, depth)].set(pos, ids, total_size, depth + 1);

	auto &entries = get_leaf();
	const size_t old_size = entries.size();
	if (!entries.set(pos, ids))
		return false;

	total_size = total_size - old_size + entries.size();
	if (entries.size() > LEAF_TARGET && depth < 3)
		split(depth);

	return true;
}

bool AppliedNodeModifiersList::Bucket::remove(v3s16 pos, u16 id, size_t &total_size, u8 depth)
{
	if (auto *branch = try_get_children())
		return branch[coordinate(pos, depth)].remove(pos, id, total_size, depth + 1);

	auto &entries = get_leaf();
	const size_t old_size = entries.size();
	if (!entries.remove(pos, id))
		return false;

	total_size = total_size - old_size + entries.size();

	return true;
}

bool AppliedNodeModifiersList::Bucket::remove(v3s16 pos, size_t &total_size, u8 depth)
{
	if (auto *branch = try_get_children())
		return branch[coordinate(pos, depth)].remove(pos, total_size, depth + 1);

	auto &entries = get_leaf();
	const size_t old_size = entries.size();
	if (!entries.remove(pos))
		return false;

	total_size = total_size - old_size + entries.size();

	return true;
}

bool AppliedNodeModifiersList::add(v3s16 pos, u16 id,
		const NodeModifierManager *prune_manager)
{
	assert(isInArea(pos, MAP_BLOCKSIZE));
	return m_root.add(pos, id, m_size, prune_manager);
}

bool AppliedNodeModifiersList::set(v3s16 pos, const std::vector<u16> &ids)
{
	assert(isInArea(pos, MAP_BLOCKSIZE));
	return m_root.set(pos, ids, m_size);
}

bool AppliedNodeModifiersList::remove(v3s16 pos, u16 id)
{
	assert(isInArea(pos, MAP_BLOCKSIZE));
	return m_root.remove(pos, id, m_size);
}

bool AppliedNodeModifiersList::remove(v3s16 pos)
{
	assert(isInArea(pos, MAP_BLOCKSIZE));
	return m_root.remove(pos, m_size);
}

void AppliedNodeModifiersList::serialize(std::ostream &os, u16 version,
		bool disk, const NameIdMapping &nimap, const IdIdMapping *id_mapping) const
{
	assert(m_size <= MAP_BLOCKSIZE * MAP_BLOCKSIZE * MAP_BLOCKSIZE * MAX_NODE_MODIFIERS);

	writeU32(os, m_size);

	if (empty())
		return;

	if (disk)
		nimap.serialize(os);

	const bool narrow_ids = disk && nimap.size() <= 256;
	m_root.forEachLeaf([&](const Leaf &leaf) {

		auto first = leaf.begin();
		while (first != leaf.end()) {

			auto last = first + 1;
			while (last != leaf.end() && last->pos == first->pos)
				++last;

			const auto p = first->pos;

			assert(last == leaf.end() || first->pos <= last->pos);
			assert(isInArea(p, MAP_BLOCKSIZE) &&
					static_cast<size_t>(last - first) <= MAX_NODE_MODIFIERS);

			writeU16(os, ((last - first - 1) << 12) | (p.Z << 8) | (p.Y << 4) | p.X);

			for (; first != last; ++first) {
				const u16 id = id_mapping ? id_mapping->get(first->id) : first->id;
				assert(!disk || id < nimap.size());

				if (narrow_ids)
					writeU8(os, id);
				else
					writeU16(os, id);
			}
		}
	});
}

void AppliedNodeModifiersList::deSerialize(std::istream &is, u16 version,
		bool disk, NameIdMapping &nimap)
{
	const u32 size = readU32(is);
	if (size > MAP_BLOCKSIZE * MAP_BLOCKSIZE * MAP_BLOCKSIZE * MAX_NODE_MODIFIERS)
		throw SerializationError("Too many node modifier entries");

	clear();
	nimap.clear();

	if (!size)
		return;

	if (disk)
		nimap.deSerialize(is);

	const bool narrow_ids = disk && nimap.size() <= 256;
	std::vector<AppliedNodeModifier> entries;
	entries.reserve(size);

	v3s16 previous;
	u32 remaining = size;
	while (remaining) {
		const u16 header = readU16(is);
		const v3s16 pos(header & 15, (header >> 4) & 15, (header >> 8) & 15);
		const u16 count = (header >> 12) + 1;

		if (!entries.empty() && previous >= pos)
			throw SerializationError("Node modifier positions are not sorted or unique");
		if (count > remaining)
			throw SerializationError("Node modifier group exceeds remaining entry count");

		remaining -= count;
		for (u16 j = 0; j < count; ++j)
			entries.push_back({pos, narrow_ids ? u16(readU8(is)) : readU16(is)});
		previous = pos;
	}

	if (entries.size() <= LEAF_TARGET)
		m_root.data = Leaf(std::move(entries));
	else
		m_root.assignRange(entries.begin(), entries.end(), 0);
	m_size = size;
}

void AppliedNodeModifiersList::remapIds(const std::function<u16(u16)> &map_id)
{
	m_root.forEachLeaf([&](Leaf &leaf) {
		leaf.remapIds(map_id);
	});
}

AppliedNodeModifiersList::ConstRange AppliedNodeModifiersList::findRange(v3s16 pos) const
{
	assert(isInArea(pos, MAP_BLOCKSIZE));

	u8 depth = 0;
	const auto &leaf = m_root.findLeaf(pos, depth).get_leaf();
	return leaf.findRange(pos);
}

// Used for merging several block-specific lists into one mesh-specific list
void AppliedNodeModifiersSnapshot::append(const AppliedNodeModifiersList &other, v3s16 offset)
{
	auto cmp_func = [](const AppliedNodeModifier &a, const AppliedNodeModifier &b) {
		return a.pos < b.pos;
	};

	const auto split = m_list.size();
	if (split + other.size() > m_list.capacity()) {
		/* `m_list.capacity() * 2` is to preserve geometric growth
		 * and avoid reallocation for each block */
		m_list.reserve(std::max(split + other.size(), m_list.capacity() * 2));
	}

	other.forEach([&](const AppliedNodeModifier &entry) {
		m_list.push_back({entry.pos + offset, entry.id});
	});

	if (split != 0 && split != m_list.size() &&
			cmp_func(m_list[split], m_list[split - 1]))
		std::inplace_merge(m_list.begin(), m_list.begin() + split, m_list.end(), cmp_func);
}

AppliedNodeModifiersSnapshot::ConstRange AppliedNodeModifiersSnapshot::findRange(v3s16 pos) const
{
	return findModifierRange(m_list, pos);
}

NodePropertyOverrides AppliedNodeModifiersSnapshot::resolve(v3s16 pos,
		const NodeModifierManager &manager) const
{
	return resolveInRange(findRange(pos), manager);
}
