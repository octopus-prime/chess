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
    static constexpr size_t SIZE  = 16384; // 2^14
    // static constexpr size_t SIZE  = 1 << 20; // 2^20 = 1M entries per table, 4MB total (3 tables)
    // Entry clamp — entries stay in [-LIMIT, +LIMIT]
    static constexpr int    LIMIT = 256;
    // Divisor when applying to eval — max adjustment per table is ±LIMIT/DIV = ±16 cp
    static constexpr int    DIV   = 16;

    using pawn_table_t    = std::array<std::array<int16_t, SIZE>, SIDE_MAX>;
    using nonpawn_table_t = std::array<std::array<int16_t, SIZE>, SIDE_MAX>;
    using cont_table_t    = std::array<std::array<int16_t, SQUARE_MAX>, TYPE_MAX>;

    correction_t()
        : pawn_table{std::make_unique<pawn_table_t>()}
        , nonpawn_table{std::make_unique<nonpawn_table_t>()}
        , cont_table{std::make_unique<cont_table_t>()}
    {
        clear();
    }

    void clear() noexcept {
        for (auto& t : *pawn_table)    t.fill(0);
        for (auto& t : *nonpawn_table) t.fill(0);
        for (auto& t : *cont_table)    t.fill(0);
    }

    // Returns the correction delta to add to static eval (centipawns)
    int get(hash_t pawn_h, hash_t nonpawn_h,
            side_e side,
            type_e last_type, square last_to) const noexcept {
        int cv = (*pawn_table)[side][pawn_h & (SIZE - 1)]
               + (*nonpawn_table)[side][nonpawn_h & (SIZE - 1)];
        if (last_type != NO_TYPE)
            cv += (*cont_table)[last_type][last_to];
        return cv / DIV;
    }

    // Call after each main-search node (not qsearch).
    // bonus = clamp((best_score - static_eval) * depth / 8, -LIMIT/4, +LIMIT/4)
    void update(hash_t pawn_h, hash_t nonpawn_h,
                side_e side,
                type_e last_type, square last_to,
                int bonus) noexcept {
        bonus = std::clamp(bonus, -LIMIT / 4, LIMIT / 4);
        update_entry((*pawn_table)[side][pawn_h & (SIZE - 1)], bonus);
        update_entry((*nonpawn_table)[side][nonpawn_h & (SIZE - 1)], bonus);
        if (last_type != NO_TYPE)
            update_entry((*cont_table)[last_type][last_to], bonus);
    }

private:
    std::unique_ptr<pawn_table_t>    pawn_table;
    std::unique_ptr<nonpawn_table_t> nonpawn_table;
    std::unique_ptr<cont_table_t>    cont_table;

    static void update_entry(int16_t& entry, int bonus) noexcept {
        // Gravity update: recent errors matter, extreme swings are dampened
        entry += static_cast<int16_t>(bonus - entry * std::abs(bonus) / LIMIT);
    }
};

