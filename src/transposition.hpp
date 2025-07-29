#pragma once

#include "node.hpp"
#include "position.hpp"

enum flag_t : std::uint8_t {
	UNKNOWN,
	UPPER,
	LOWER,
	EXACT
	//		EGTB
};

struct entry_t {
	size_t hash;	//8
	move_t move;	//4
	int16_t score;	//2
	flag_t flag;	//1
	uint8_t	depth;	//1
};

class transposition_t {
	std::vector<entry_t> entries;

public:
	transposition_t(size_t size) : entries(size) {
	}

	void clear() noexcept {
		std::ranges::fill(entries, entry_t{});
	}

	template <side_e side>
	void put(const node& node, move_t move, int16_t score, flag_t flag, uint8_t depth) noexcept {
		entry_t& entry = entries[node.hash<side>() % entries.size()];
		if ((depth >= entry.depth))// || (depth == entry.depth && score > entry.score))
			entry = entry_t{node.hash<side>(), move, score, flag, depth};
	}

	template <side_e side>
	const entry_t* get(const node& node) const noexcept {
		const entry_t& entry = entries[node.hash<side>() % entries.size()];
		if (entry.hash == node.hash<side>())
			return &entry;
		else
			return nullptr;
	}

	size_t full() const noexcept {
		return 1000.0 * std::ranges::count_if(entries, [](auto&& e){ return e.flag != UNKNOWN; }) / entries.size();
	}
};

struct entry2_t {
	hash_t hash;	//8
	move2_t move;	//4
	int16_t score;	//2
	flag_t flag;	//1
	uint8_t	depth;	//1
};

class transposition2_t {
	std::vector<entry2_t> entries;

public:
	transposition2_t() : entries(104'395'303) {
	}

	// transposition2_t() : entries(15'485'863) {
	// }

	transposition2_t(size_t size) : entries(size) {
	}

	void clear() noexcept {
		std::ranges::fill(entries, entry2_t{});
	}

	void put(hash_t hash, move2_t move, int16_t score, flag_t flag, uint8_t depth) noexcept {
		entry2_t& entry = entries[hash % entries.size()];
		if ((depth >= entry.depth))// || (depth == entry.depth && score > entry.score))
			entry = entry2_t{hash, move, score, flag, depth};
	}

	const entry2_t* get(hash_t hash) const noexcept {
		const entry2_t& entry = entries[hash % entries.size()];
		if (entry.hash == hash)
			return &entry;
		else
			return nullptr;
	}

	size_t full() const noexcept {
		return 1000.0 * std::ranges::count_if(entries, [](auto&& e){ return e.flag != UNKNOWN; }) / entries.size();
	}
};
