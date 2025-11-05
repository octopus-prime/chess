// #pragma once

// #include "hashes.hpp"
// #include "move.hpp"

// enum flag_t : std::uint8_t {
// 	UNKNOWN,
// 	UPPER,
// 	LOWER,
// 	EXACT
// 	//		EGTB
// };

// struct entry_t {
// 	hash_t hash;	//8
// 	move_t move;	//4
// 	int16_t score;	//2
// 	flag_t flag;	//1
// 	uint8_t	depth;	//1
// };

// class transposition_t {
// 	std::vector<entry_t> entries;

// public:
// 	// transposition_t() : entries(104'395'303) {
// 	// }

// 	// transposition_t() : entries(15'485'863) {
// 	// }

// 	transposition_t() : entries(1'000'037) {
// 	}

// 	// transposition_t(size_t size) : entries(size) {
// 	// }

// 	void clear() noexcept {
// 		std::ranges::fill(entries, entry_t{});
// 	}

// 	void put(hash_t hash, move_t move, int16_t score, flag_t flag, uint8_t depth) noexcept {
// 		entry_t& entry = entries[hash % entries.size()];
//         if (entry.flag == UNKNOWN || entry.hash == hash || depth >= entry.depth) {
//             entry = {hash, move, score, flag, depth};
//         }
// 	}

//     const entry_t* get(hash_t hash) const noexcept {
//         const entry_t& entry = entries[hash % entries.size()];
//         return (entry.hash == hash) ? &entry : nullptr;
//     }

// 	size_t full() const noexcept {
// 		return 1000.0 * std::ranges::count_if(entries, [](auto&& e){ return e.flag != UNKNOWN; }) / entries.size();
// 	}
// };

// #pragma once

// #include "hashes.hpp"
// #include "move.hpp"

// enum flag_t : std::uint8_t {
// 	UNKNOWN,
// 	UPPER,
// 	LOWER,
// 	EXACT
// 	//		EGTB
// };

// struct entry_t {
// 	hash_t hash;	//8
// 	move_t move;	//4
// 	int16_t score;	//2
// 	flag_t flag;	//1
// 	uint8_t	depth;	//1
// };

// class transposition_t {
// 	constexpr static size_t BUCKET_SIZE = 4;

// 	struct alignas(64) bucket_t {
// 		entry_t entries[BUCKET_SIZE];
// 	};

// 	static_assert(sizeof(bucket_t) == 64);

//     std::vector<bucket_t> buckets;

// public:
//     // transposition_t() : buckets(1'000'037) {}
// 	transposition_t() : buckets(250'007) {}

//     void clear() noexcept {
//         std::ranges::fill(buckets, bucket_t{});
//     }

//     void put(hash_t hash, move_t move, int16_t score, flag_t flag, uint8_t depth) noexcept {
//         bucket_t& bucket = buckets[hash % buckets.size()];
// 		entry_t* entry = std::ranges::find(bucket.entries, hash, &entry_t::hash);
// 		if (entry == std::end(bucket.entries)) {
// 			entry = std::ranges::min_element(bucket.entries, {}, &entry_t::depth);
// 		}
// 		if (depth >= entry->depth) {
// 			*entry = {hash, move, score, flag, depth};
// 		}
//     }

//     const entry_t* get(hash_t hash) const noexcept {
//         const bucket_t& bucket = buckets[hash % buckets.size()];
// 		const entry_t* entry = std::ranges::find(bucket.entries, hash, &entry_t::hash);
// 		return entry != std::end(bucket.entries) ? entry : nullptr;
//     }

//     size_t full() const noexcept {
// 		size_t count = std::ranges::fold_left(buckets, 0ull, [](size_t acc, const bucket_t& bucket) {
// 			return acc + std::ranges::count(bucket.entries, UNKNOWN, &entry_t::flag);
// 		});
//         return 1000.0 - 1000.0 * count / (buckets.size() * BUCKET_SIZE);
//     }
// };

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

// struct tt_move_t {
// 	uint16_t from : 6;
// 	uint16_t to : 6;
// 	uint16_t promotion : 4;

// 	constexpr tt_move_t() noexcept : from{0}, to{0}, promotion{0} {
// 	}

// 	constexpr tt_move_t(square from, square to, type_e promotion = NO_TYPE) noexcept
// 		: from{static_cast<uint16_t>(from)}, to{static_cast<uint16_t>(to)}, promotion{static_cast<uint16_t>(promotion)} {
// 	}

// 	constexpr tt_move_t(move_t move) noexcept
// 		: from{static_cast<uint16_t>(move.from())}, to{static_cast<uint16_t>(move.to())}, promotion{static_cast<uint16_t>(move.promotion())} {
// 	}

// 	constexpr operator move_t() const noexcept {
// 		return move_t{static_cast<square_e>(from), static_cast<square_e>(to), static_cast<type_e>(promotion)};
// 	}

// 	constexpr bool operator==(const tt_move_t& other) const noexcept = default;
// };

struct entry_t {
	uint16_t key;	//2
	move_t move;	//2
	int16_t score;	//2
	flag_t flag;	//1
	uint8_t	depth;	//1
};

static_assert(sizeof(entry_t) == 8);

class transposition_t {
	constexpr static size_t BUCKET_SIZE = 64 / sizeof(entry_t);

	static_assert(BUCKET_SIZE == 8);

	struct alignas(64) bucket_t {
		entry_t entries[BUCKET_SIZE]{};
	};

	static_assert(sizeof(bucket_t) == 64);

    std::vector<bucket_t> buckets;

public:
    // transposition_t() : buckets(1'000'037) {}
    // transposition_t() : buckets(499'999) {}
    transposition_t() : buckets(250'007) {}
    // transposition_t() : buckets(125'003) {}

    void clear() noexcept {
        std::ranges::fill(buckets, bucket_t{});
    }

    void put(hash_t hash, move_t move, int16_t score, flag_t flag, uint8_t depth) noexcept {
        bucket_t& bucket = buckets[hash % buckets.size()];
		uint16_t key = static_cast<uint16_t>(hash);
		entry_t* entry = std::ranges::find(bucket.entries, key, &entry_t::key);
		if (entry == std::end(bucket.entries)) {
			entry = std::ranges::min_element(bucket.entries, {}, &entry_t::depth);
		}
		*entry = {key, move, score, flag, depth};
    }

    // const entry_t* get(hash_t hash) const noexcept {
    //     const bucket_t& bucket = buckets[hash % buckets.size()];
	// 	uint16_t key = static_cast<uint16_t>(hash);
	// 	const entry_t* entry = std::ranges::find(bucket.entries, key, &entry_t::key);
	// 	return entry != std::end(bucket.entries) ? entry : nullptr;
    // }

    std::optional<entry_t> get(hash_t hash) const noexcept {
        const bucket_t& bucket = buckets[hash % buckets.size()];
		uint16_t key = static_cast<uint16_t>(hash);
		const entry_t* entry = std::ranges::find(bucket.entries, key, &entry_t::key);
		if (entry == std::end(bucket.entries))
			return std::nullopt;
		return *entry;
    }

    size_t full() const noexcept {
		size_t count = std::ranges::fold_left(buckets, 0ull, [](size_t acc, const bucket_t& bucket) {
			return acc + std::ranges::count(bucket.entries, UNKNOWN, &entry_t::flag);
		});
        return 1000.0 - 1000.0 * count / (buckets.size() * BUCKET_SIZE);
    }
};
