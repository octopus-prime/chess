#pragma once

#include "hashes.hpp"
#include "move.hpp"

enum flag_t : std::uint8_t {
	UNKNOWN,
	UPPER,
	LOWER,
	EXACT
	//		EGTB
};

struct entry_t {
	hash_t hash;	//8
	move_t move;	//4
	int16_t score;	//2
	flag_t flag;	//1
	uint8_t	depth;	//1
};

class transposition_t {
	std::vector<entry_t> entries;

public:
	// transposition2_t() : entries(104'395'303) {
	// }

	// transposition2_t() : entries(15'485'863) {
	// }

	transposition_t() : entries(1'000'037) {
	}

	// transposition_t(size_t size) : entries(size) {
	// }

	void clear() noexcept {
		std::ranges::fill(entries, entry_t{});
	}

	void put(hash_t hash, move_t move, int16_t score, flag_t flag, uint8_t depth) noexcept {
		entry_t& entry = entries[hash % entries.size()];
        if (entry.flag == UNKNOWN || entry.hash == hash || depth > entry.depth) {
            entry = {hash, move, score, flag, depth};
        }
	}

    const entry_t* get(hash_t hash) const noexcept {
        const entry_t& entry = entries[hash % entries.size()];
        return (entry.hash == hash) ? &entry : nullptr;
    }

	size_t full() const noexcept {
		return 1000.0 * std::ranges::count_if(entries, [](auto&& e){ return e.flag != UNKNOWN; }) / entries.size();
	}
};
