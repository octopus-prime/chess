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
        ut::expect(ut::eq(corr.get(pos.pawn_hash(), pos.minor_hash(), pos.major_hash(),
                                   pos.get_side(), NO_TYPE, square{A1}), 0));
        // Same after explicit clear
        corr.clear();
        ut::expect(ut::eq(corr.get(pos.pawn_hash(), pos.minor_hash(), pos.major_hash(),
                                   pos.get_side(), NO_TYPE, square{A1}), 0));
    };

    "correction_update_get"_test = [] {
        // update_entry: entry += bonus - entry * |bonus| / LIMIT
        // LIMIT = 256, DIV = 16
        // Starting 0, bonus = 64 (= LIMIT/4):
        //   entry = 0 + 64 - 0*64/256 = 64  (pawn + minor + major slots all updated)
        // get() = (64 + 64 + 64) / 16 = 12
        correction_t corr{};
        position_t pos{};
        auto ph  = pos.pawn_hash();
        auto mnh = pos.minor_hash();
        auto mjh = pos.major_hash();
        auto side = pos.get_side();

        corr.update(ph, mnh, mjh, side, NO_TYPE, square{A1}, 64);
        ut::expect(ut::eq(corr.get(ph, mnh, mjh, side, NO_TYPE, square{A1}), 12));

        // Second update, bonus = 64:
        //   entry = 64 + 64 - 64*64/256 = 112
        // get() = (112 + 112 + 112) / 16 = 21
        corr.update(ph, mnh, mjh, side, NO_TYPE, square{A1}, 64);
        ut::expect(ut::eq(corr.get(ph, mnh, mjh, side, NO_TYPE, square{A1}), 21));
    };

    "correction_negative_bonus"_test = [] {
        // Negative bonus drives all entries negative
        correction_t corr{};
        position_t pos{};
        auto ph  = pos.pawn_hash();
        auto mnh = pos.minor_hash();
        auto mjh = pos.major_hash();
        auto side = pos.get_side();

        corr.update(ph, mnh, mjh, side, NO_TYPE, square{A1}, -64);
        // entry = -64 for each of pawn/minor/major
        // get() = (-64 + -64 + -64) / 16 = -12
        ut::expect(ut::eq(corr.get(ph, mnh, mjh, side, NO_TYPE, square{A1}), -12));
    };

    "correction_continuation"_test = [] {
        // With a valid last_type the cont_table slot is also updated
        correction_t corr{};
        position_t pos{};
        auto ph  = pos.pawn_hash();
        auto mnh = pos.minor_hash();
        auto mjh = pos.major_hash();
        auto side = pos.get_side();

        corr.update(ph, mnh, mjh, side, KNIGHT, square{F3}, 64);
        // pawn=64, minor=64, major=64, cont[KNIGHT][F3]=64
        // get() = (64 + 64 + 64 + 64) / 16 = 16
        ut::expect(ut::eq(corr.get(ph, mnh, mjh, side, KNIGHT, square{F3}), 16));

        // Query with NO_TYPE must not include the cont slot → (64+64+64) / 16 = 12
        ut::expect(ut::eq(corr.get(ph, mnh, mjh, side, NO_TYPE, square{F3}), 12));
    };

    "correction_different_positions"_test = [] {
        // Different pawn structures must index into distinct slots
        correction_t corr{};

        position_t pos1{};
        pos1.make_move("e2e4"_m);
        pos1.make_move("e7e5"_m);

        position_t pos2{};
        pos2.make_move("d2d4"_m);  // different pawn structure
        pos2.make_move("d7d5"_m);

        ut::expect(ut::neq(pos1.pawn_hash(), pos2.pawn_hash()));

        corr.update(pos1.pawn_hash(), pos1.minor_hash(), pos1.major_hash(), pos1.get_side(), NO_TYPE, square{A1}, 64);
        ut::expect(ut::neq(corr.get(pos1.pawn_hash(), pos1.minor_hash(), pos1.major_hash(), pos1.get_side(), NO_TYPE, square{A1}), 0));
    };

    "correction_clear_resets"_test = [] {
        correction_t corr{};
        position_t pos{};
        auto ph  = pos.pawn_hash();
        auto mnh = pos.minor_hash();
        auto mjh = pos.major_hash();
        auto side = pos.get_side();

        for (int i = 0; i < 50; ++i)
            corr.update(ph, mnh, mjh, side, NO_TYPE, square{A1}, 64);

        ut::expect(ut::neq(corr.get(ph, mnh, mjh, side, NO_TYPE, square{A1}), 0));
        corr.clear();
        ut::expect(ut::eq(corr.get(ph, mnh, mjh, side, NO_TYPE, square{A1}), 0));
    };

    "correction_clamp"_test = [] {
        // Bonus is clamped to [-LIMIT/4, +LIMIT/4] = [-64, +64] inside update()
        correction_t corr1{};
        correction_t corr2{};
        position_t pos{};
        auto ph  = pos.pawn_hash();
        auto mnh = pos.minor_hash();
        auto mjh = pos.major_hash();
        auto side = pos.get_side();

        corr1.update(ph, mnh, mjh, side, NO_TYPE, square{A1}, 64);    // exactly at clamp
        corr2.update(ph, mnh, mjh, side, NO_TYPE, square{A1}, 10000); // way above → clamped to 64

        ut::expect(ut::eq(corr1.get(ph, mnh, mjh, side, NO_TYPE, square{A1}),
                          corr2.get(ph, mnh, mjh, side, NO_TYPE, square{A1})));
    };

}
