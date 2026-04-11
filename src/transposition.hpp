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
	uint32_t key;	//4
	move_t move;	//2
	int16_t score;	//2
	flag_t flag;	//1
	int8_t	depth;	//1
};

static_assert(sizeof(entry_t) == 12);

class transposition_t {
	constexpr static size_t BUCKET_SIZE = 64 / sizeof(entry_t);

	static_assert(BUCKET_SIZE == 5);

	struct alignas(64) bucket_t {
		entry_t entries[BUCKET_SIZE]{};
	};

	static_assert(sizeof(bucket_t) == 64);

    std::vector<bucket_t> buckets;
	std::size_t used{0};

public:
    // transposition_t() : buckets(1'000'037) {}
    // transposition_t() : buckets(499'999) {}
    transposition_t() : buckets(250'007) {}
    // transposition_t() : buckets(125'003) {}

    void clear() noexcept {
        std::ranges::fill(buckets, bucket_t{});
		used = 0;
    }

    void put(hash_t hash, move_t move, int16_t score, flag_t flag, int8_t depth) noexcept {
        bucket_t& bucket = buckets[hash % buckets.size()];
		uint32_t key = static_cast<uint32_t>(hash);
		entry_t* entry = std::ranges::find(bucket.entries, key, &entry_t::key);
		if (entry == std::end(bucket.entries)) {
			entry = std::ranges::min_element(bucket.entries, {}, &entry_t::depth);
			used += entry->flag == UNKNOWN;
		}
		*entry = {key, move, score, flag, depth};
    }

    std::optional<entry_t> get(hash_t hash) const noexcept {
        const bucket_t& bucket = buckets[hash % buckets.size()];
		uint32_t key = static_cast<uint32_t>(hash);
		const entry_t* entry = std::ranges::find(bucket.entries, key, &entry_t::key);
		if (entry == std::end(bucket.entries))
			return std::nullopt;
		return *entry;
    }

    size_t full() const noexcept {
		return 1000.0 * used / (buckets.size() * BUCKET_SIZE);
    }
};
