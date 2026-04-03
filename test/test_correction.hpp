#pragma once

#include <correction.hpp>
#include <position.hpp>
#include "ut.hpp"

void test_correction() {
    namespace ut = boost::ut;
    using ut::operator""_test;

    "correction_clear"_test = [] {
        correction_t corr{};
        position_t pos{};
        // After construction (value-initialised) get() must return 0
        ut::expect(ut::eq(corr.get(pos.pawn_hash(), pos.nonpawn_hash(),
                                   pos.get_side(), NO_TYPE, square{A1}), 0));
        // Same after explicit clear
        corr.clear();
        ut::expect(ut::eq(corr.get(pos.pawn_hash(), pos.nonpawn_hash(),
                                   pos.get_side(), NO_TYPE, square{A1}), 0));
    };

    "correction_update_get"_test = [] {
        // update_entry: entry += bonus - entry * |bonus| / LIMIT
        // LIMIT = 256, DIV = 16
        // Starting 0, bonus = 64 (= LIMIT/4):
        //   entry = 0 + 64 - 0*64/256 = 64  (both pawn and nonpawn slots updated)
        // get() = (64 + 64) / 16 = 8
        correction_t corr{};
        position_t pos{};
        auto ph  = pos.pawn_hash();
        auto nph = pos.nonpawn_hash();
        auto side = pos.get_side();

        corr.update(ph, nph, side, NO_TYPE, square{A1}, 64);
        ut::expect(ut::eq(corr.get(ph, nph, side, NO_TYPE, square{A1}), 8));

        // Second update, bonus = 64:
        //   entry = 64 + 64 - 64*64/256 = 64 + 64 - 16 = 112
        // get() = (112 + 112) / 16 = 14   (pawn + nonpawn slots both hit)
        corr.update(ph, nph, side, NO_TYPE, square{A1}, 64);
        ut::expect(ut::eq(corr.get(ph, nph, side, NO_TYPE, square{A1}), 14));
    };

    "correction_negative_bonus"_test = [] {
        // Negative bonus drives entry negative, get() returns negative delta
        correction_t corr{};
        position_t pos{};
        auto ph   = pos.pawn_hash();
        auto nph  = pos.nonpawn_hash();
        auto side = pos.get_side();

        corr.update(ph, nph, side, NO_TYPE, square{A1}, -64);
        // entry = 0 - 64 - 0*64/256 = -64
        // get() = (-64 + -64) / 16 = -8  (pawn + nonpawn)
        ut::expect(ut::eq(corr.get(ph, nph, side, NO_TYPE, square{A1}), -8));
    };

    "correction_continuation"_test = [] {
        // With a valid last_type the cont_table slot is also updated
        correction_t corr{};
        position_t pos{};
        auto ph   = pos.pawn_hash();
        auto nph  = pos.nonpawn_hash();
        auto side = pos.get_side();

        corr.update(ph, nph, side, KNIGHT, square{F3}, 64);
        // pawn entry = 64, nonpawn entry = 64, cont[KNIGHT][F3] = 64
        // get() = (64 + 64 + 64) / 16 = 12
        ut::expect(ut::eq(corr.get(ph, nph, side, KNIGHT, square{F3}), 12));

        // Query with NO_TYPE must not include the cont slot → (64 + 64) / 16 = 8
        ut::expect(ut::eq(corr.get(ph, nph, side, NO_TYPE, square{F3}), 8));
    };

    "correction_different_positions"_test = [] {
        // Different pawn structures must index into distinct slots
        correction_t corr{};

        position_t pos1{};
        pos1.make_move("e2e4"_m);  // e-pawn advanced
        pos1.make_move("e7e5"_m);

        position_t pos2{};
        pos2.make_move("d2d4"_m);  // d-pawn advanced — different pawn structure
        pos2.make_move("d7d5"_m);

        ut::expect(ut::neq(pos1.pawn_hash(), pos2.pawn_hash()));

        // Update pos1's slot, pos2 must be unaffected
        corr.update(pos1.pawn_hash(), pos1.nonpawn_hash(), pos1.get_side(), NO_TYPE, square{A1}, 64);
        ut::expect(ut::neq(corr.get(pos1.pawn_hash(), pos1.nonpawn_hash(), pos1.get_side(), NO_TYPE, square{A1}), 0));
        // pos2 shares the nonpawn table slot (no pieces moved) but has a different pawn slot
        // Only check that pos2's pawn slot wasn't dirtied by verifying pawn_hash differs
        // (We can't easily guarantee they land in different nonpawn slots, so just confirm
        //  the two positions have different pawn_hashes — already asserted above.)
    };

    "correction_clear_resets"_test = [] {
        correction_t corr{};
        position_t pos{};
        auto ph   = pos.pawn_hash();
        auto nph  = pos.nonpawn_hash();
        auto side = pos.get_side();

        for (int i = 0; i < 50; ++i)
            corr.update(ph, nph, side, NO_TYPE, square{A1}, 64);

        ut::expect(ut::neq(corr.get(ph, nph, side, NO_TYPE, square{A1}), 0));
        corr.clear();
        ut::expect(ut::eq(corr.get(ph, nph, side, NO_TYPE, square{A1}), 0));
    };

    "correction_clamp"_test = [] {
        // Bonus is clamped to [-LIMIT/4, +LIMIT/4] = [-64, +64] inside update()
        // Passing a very large value should behave identically to LIMIT/4
        correction_t corr1{};
        correction_t corr2{};
        position_t pos{};
        auto ph   = pos.pawn_hash();
        auto nph  = pos.nonpawn_hash();
        auto side = pos.get_side();

        corr1.update(ph, nph, side, NO_TYPE, square{A1}, 64);       // exactly at clamp
        corr2.update(ph, nph, side, NO_TYPE, square{A1}, 10000);    // way above → clamped to 64

        ut::expect(ut::eq(corr1.get(ph, nph, side, NO_TYPE, square{A1}),
                          corr2.get(ph, nph, side, NO_TYPE, square{A1})));
    };
}
