#pragma once

#include "node.hpp"

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

constexpr int prime = 99527; // 997

class transposition_t {
	std::vector<entry_t> entries;

public:
	transposition_t(size_t size) : entries(size /*+ prime*/) {
	}

	void clear() noexcept {
		std::ranges::fill(entries, entry_t{});
	}

	template <side_e side>
	void put(const node& node, move_t move, int16_t score, flag_t flag, uint8_t depth) noexcept {
		entry_t& entry = entries[node.hash<side>() % entries.size()];
		if ((depth > entry.depth) || (depth == entry.depth && score > entry.score))
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
};
