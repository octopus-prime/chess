#pragma once

#include "hashes.hpp"
#include "piece.hpp"
#include "square.hpp"
#include "side.hpp"
#include <array>
#include <memory>
#include <algorithm>
#include <cstdint>
#include <cmath>

struct correction_t {

    // Table size — power of 2 for fast masking
    static constexpr size_t SIZE  = 1 << 16;
    // Entry clamp — entries stay in [-LIMIT, +LIMIT]
    static constexpr int    LIMIT = 256;
    // Divisor when applying to eval — max adjustment per table is ±LIMIT/DIV = ±16 cp
    static constexpr int    DIV   = 16;

    using pawn_table_t  = std::array<std::array<int16_t, SIZE>, SIDE_MAX>;
    using minor_table_t = std::array<std::array<int16_t, SIZE>, SIDE_MAX>;
    using major_table_t = std::array<std::array<int16_t, SIZE>, SIDE_MAX>;
    using cont_table_t  = std::array<std::array<int16_t, SQUARE_MAX>, TYPE_MAX>;

    correction_t()
        : pawn_table{std::make_unique<pawn_table_t>()}
        , minor_table{std::make_unique<minor_table_t>()}
        , major_table{std::make_unique<major_table_t>()}
        , cont_table{std::make_unique<cont_table_t>()}
    {
        clear();
    }

    void clear() noexcept {
        for (auto& t : *pawn_table)  t.fill(0);
        for (auto& t : *minor_table) t.fill(0);
        for (auto& t : *major_table) t.fill(0);
        for (auto& t : *cont_table)  t.fill(0);
    }

    // Returns the correction delta to add to static eval (centipawns)
    int get(hash_t pawn_h, hash_t minor_h, hash_t major_h,
            side_e side,
            type_e last_type, square last_to) const noexcept {
        int cv = 15 * (*pawn_table)[side][pawn_h   & (SIZE - 1)]
               + 10 * (*minor_table)[side][minor_h & (SIZE - 1)]
               + 10 * (*major_table)[side][major_h & (SIZE - 1)];
        if (last_type != NO_TYPE)
            cv += 20 * (*cont_table)[last_type][last_to];
        return cv / (DIV * DIV);
    }

    // Call after each main-search node (not qsearch).
    void update(hash_t pawn_h, hash_t minor_h, hash_t major_h,
                side_e side,
                type_e last_type, square last_to,
                int bonus) noexcept {
        bonus = std::clamp(bonus, -LIMIT / 4, LIMIT / 4);
        update_entry((*pawn_table)[side][pawn_h   & (SIZE - 1)], bonus);
        update_entry((*minor_table)[side][minor_h & (SIZE - 1)], bonus);
        update_entry((*major_table)[side][major_h & (SIZE - 1)], bonus);
        if (last_type != NO_TYPE)
            update_entry((*cont_table)[last_type][last_to], bonus);
    }

private:
    std::unique_ptr<pawn_table_t>  pawn_table;
    std::unique_ptr<minor_table_t> minor_table;
    std::unique_ptr<major_table_t> major_table;
    std::unique_ptr<cont_table_t>  cont_table;

    static void update_entry(int16_t& entry, int bonus) noexcept {
        // Gravity update: recent errors matter, extreme swings are dampened
        entry += static_cast<int16_t>(bonus - entry * std::abs(bonus) / LIMIT);
    }
};

